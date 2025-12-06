#include "nbody.hpp"
#include <iostream>
#include <fstream>
#include <random>
#include <cmath>
#include <iomanip>
#include <algorithm>

// Utility function to get wall time
double get_wall_time() {
    return MPI_Wtime();
}

// MPI datatype for Particle
void setup_mpi_particle_type(MPI_Datatype* particle_type) {
    const int nitems = 5;
    int blocklengths[5] = {3, 3, 3, 1, 1};
    MPI_Datatype types[5] = {MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE, MPI_INT};
    MPI_Aint offsets[5];

    offsets[0] = offsetof(Particle, position);
    offsets[1] = offsetof(Particle, velocity);
    offsets[2] = offsetof(Particle, acceleration);
    offsets[3] = offsetof(Particle, mass);
    offsets[4] = offsetof(Particle, id);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, particle_type);
    MPI_Type_commit(particle_type);
}

// Constructor
NBodySimulator::NBodySimulator(const SimulationParams& params_, MPI_Comm comm_)
    : params(params_), comm(comm_), total_time(0), compute_time(0),
      communication_time(0), tree_build_time(0), total_particle_updates(0) {

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    // Define global domain
    double half_size = params.domain_size / 2.0;
    global_domain = BoundingBox(
        Vec3(-half_size, -half_size, -half_size),
        Vec3(half_size, half_size, half_size)
    );

    start_time = get_wall_time();
}

NBodySimulator::~NBodySimulator() {
    // Cleanup
}

void NBodySimulator::generate_initial_conditions(std::vector<Particle>& particles) {
    // Generate particles in a galaxy-like distribution (Plummer model)
    std::mt19937 gen(42 + rank);  // Reproducible seed
    std::uniform_real_distribution<> dis(0.0, 1.0);
    std::normal_distribution<> normal(0.0, 1.0);

    particles.resize(params.n_particles);

    double total_mass = 1e12;  // Solar masses
    double plummer_radius = 1.0;  // Scale length

    for (int i = 0; i < params.n_particles; ++i) {
        // Plummer sphere sampling
        double radius = plummer_radius / std::sqrt(std::pow(dis(gen), -2.0/3.0) - 1.0);
        double theta = std::acos(2.0 * dis(gen) - 1.0);
        double phi = 2.0 * M_PI * dis(gen);

        Vec3 pos(
            radius * std::sin(theta) * std::cos(phi),
            radius * std::sin(theta) * std::sin(phi),
            radius * std::cos(theta)
        );

        // Simple velocity initialization (can be improved with virial theorem)
        Vec3 vel(
            normal(gen) * 0.1,
            normal(gen) * 0.1,
            normal(gen) * 0.1
        );

        particles[i] = Particle(pos, vel, total_mass / params.n_particles, i);
    }

    if (rank == 0) {
        std::cout << "Generated " << params.n_particles << " particles in Plummer distribution\n";
    }
}

void NBodySimulator::decompose_domain() {
    // Simple 1D slab decomposition along X-axis
    // Can be improved to 3D decomposition for better load balance

    double domain_width = global_domain.max.x - global_domain.min.x;
    double slab_width = domain_width / size;

    local_domain = global_domain;
    local_domain.min.x = global_domain.min.x + rank * slab_width;
    local_domain.max.x = global_domain.min.x + (rank + 1) * slab_width;

    if (rank == 0) {
        std::cout << "Domain decomposed into " << size << " slabs\n";
    }
}

void NBodySimulator::distribute_particles() {
    std::vector<Particle> all_particles;

    // Root generates and distributes
    if (rank == 0) {
        generate_initial_conditions(all_particles);
    }

    // Setup MPI datatype
    MPI_Datatype mpi_particle;
    setup_mpi_particle_type(&mpi_particle);

    // Broadcast total count (already in params, but for safety)
    int total_count = params.n_particles;
    MPI_Bcast(&total_count, 1, MPI_INT, 0, comm);

    // Allocate receive buffer
    if (rank != 0) {
        all_particles.resize(total_count);
    }

    // Broadcast all particles (for initial distribution)
    MPI_Bcast(all_particles.data(), total_count, mpi_particle, 0, comm);

    // Each rank takes particles in its domain
    local_particles.clear();
    for (const auto& p : all_particles) {
        if (local_domain.contains(p.position)) {
            local_particles.push_back(p);
        }
    }

    MPI_Type_free(&mpi_particle);

    if (rank == 0) {
        std::cout << "Particles distributed across " << size << " ranks\n";
    }

    // Print load balance
    int local_count = local_particles.size();
    int min_count, max_count;
    MPI_Allreduce(&local_count, &min_count, 1, MPI_INT, MPI_MIN, comm);
    MPI_Allreduce(&local_count, &max_count, 1, MPI_INT, MPI_MAX, comm);

    if (rank == 0) {
        std::cout << "Load balance: min=" << min_count << ", max=" << max_count
                  << ", avg=" << params.n_particles / size << "\n";
    }
}

void NBodySimulator::exchange_ghost_particles() {
    double comm_start = get_wall_time();

    // Setup MPI datatype
    MPI_Datatype mpi_particle;
    setup_mpi_particle_type(&mpi_particle);

    // For simplicity, do all-gather (can optimize to only exchange with neighbors)
    int local_count = local_particles.size();
    std::vector<int> recv_counts(size);
    std::vector<int> displs(size);

    // Gather counts from all ranks
    MPI_Allgather(&local_count, 1, MPI_INT, recv_counts.data(), 1, MPI_INT, comm);

    // Calculate displacements
    int total_particles = 0;
    for (int i = 0; i < size; ++i) {
        displs[i] = total_particles;
        total_particles += recv_counts[i];
    }

    // Allocate buffer for all particles
    ghost_particles.resize(total_particles);

    // All-gather particles
    MPI_Allgatherv(local_particles.data(), local_count, mpi_particle,
                   ghost_particles.data(), recv_counts.data(), displs.data(),
                   mpi_particle, comm);

    MPI_Type_free(&mpi_particle);

    communication_time += get_wall_time() - comm_start;
}

void NBodySimulator::compute_forces() {
    double compute_start = get_wall_time();

#ifdef USE_CUDA
    if (params.use_gpu) {
        compute_forces_gpu();
        compute_time += get_wall_time() - compute_start;
        return;
    }
#endif

    // Build Barnes-Hut tree
    double tree_start = get_wall_time();
    BarnesHutTree tree(ghost_particles, global_domain);
    tree_build_time += get_wall_time() - tree_start;

    // Calculate forces using Barnes-Hut
    tree.calculate_forces(local_particles, params.theta);

    compute_time += get_wall_time() - compute_start;
}

void NBodySimulator::integrate_particles() {
    // Leapfrog integration (kick-drift-kick)
    double dt = params.dt;

    #pragma omp parallel for
    for (size_t i = 0; i < local_particles.size(); ++i) {
        Particle& p = local_particles[i];

        // Kick: update velocity by half timestep
        p.velocity += p.acceleration * (dt * 0.5);

        // Drift: update position
        p.position += p.velocity * dt;

        // Apply periodic boundary conditions (optional)
        // Wrap around if particle leaves domain
        if (p.position.x < global_domain.min.x) p.position.x += global_domain.max.x - global_domain.min.x;
        if (p.position.x > global_domain.max.x) p.position.x -= global_domain.max.x - global_domain.min.x;
        if (p.position.y < global_domain.min.y) p.position.y += global_domain.max.y - global_domain.min.y;
        if (p.position.y > global_domain.max.y) p.position.y -= global_domain.max.y - global_domain.min.y;
        if (p.position.z < global_domain.min.z) p.position.z += global_domain.max.z - global_domain.min.z;
        if (p.position.z > global_domain.max.z) p.position.z -= global_domain.max.z - global_domain.min.z;
    }

    // Update accelerations with new forces
    compute_forces();

    #pragma omp parallel for
    for (size_t i = 0; i < local_particles.size(); ++i) {
        Particle& p = local_particles[i];
        // Final kick: update velocity by half timestep
        p.velocity += p.acceleration * (dt * 0.5);
    }

    total_particle_updates += local_particles.size();
}

void NBodySimulator::output_vtk(int step) {
    if (!params.save_vtk || step % params.output_frequency != 0) return;

    double io_start = get_wall_time();

    // Gather all particles to root for output
    MPI_Datatype mpi_particle;
    setup_mpi_particle_type(&mpi_particle);

    int local_count = local_particles.size();
    std::vector<int> recv_counts(size);
    std::vector<int> displs(size);

    MPI_Gather(&local_count, 1, MPI_INT, recv_counts.data(), 1, MPI_INT, 0, comm);

    std::vector<Particle> all_particles;
    if (rank == 0) {
        int total = 0;
        for (int i = 0; i < size; ++i) {
            displs[i] = total;
            total += recv_counts[i];
        }
        all_particles.resize(total);
    }

    MPI_Gatherv(local_particles.data(), local_count, mpi_particle,
                all_particles.data(), recv_counts.data(), displs.data(),
                mpi_particle, 0, comm);

    if (rank == 0) {
        std::string filename = params.output_file;
        size_t ext_pos = filename.find_last_of('.');
        if (ext_pos != std::string::npos) {
            filename = filename.substr(0, ext_pos) + "_" +
                      std::to_string(step) + filename.substr(ext_pos);
        } else {
            filename += "_" + std::to_string(step) + ".vtk";
        }

        std::ofstream file(filename);
        file << "# vtk DataFile Version 3.0\n";
        file << "N-Body Simulation\n";
        file << "ASCII\n";
        file << "DATASET POLYDATA\n";
        file << "POINTS " << all_particles.size() << " float\n";

        for (const auto& p : all_particles) {
            file << p.position.x << " " << p.position.y << " " << p.position.z << "\n";
        }

        // Add velocity and mass as point data
        file << "\nPOINT_DATA " << all_particles.size() << "\n";
        file << "VECTORS velocity float\n";
        for (const auto& p : all_particles) {
            file << p.velocity.x << " " << p.velocity.y << " " << p.velocity.z << "\n";
        }

        file << "\nSCALARS mass float 1\n";
        file << "LOOKUP_TABLE default\n";
        for (const auto& p : all_particles) {
            file << p.mass << "\n";
        }

        file.close();
        std::cout << "Wrote output: " << filename << "\n";
    }

    MPI_Type_free(&mpi_particle);
}

void NBodySimulator::initialize() {
    if (rank == 0) {
        std::cout << "\n=== N-Body Simulation with Barnes-Hut ===" << std::endl;
        std::cout << "Particles: " << params.n_particles << std::endl;
        std::cout << "Steps: " << params.n_steps << std::endl;
        std::cout << "Time step: " << params.dt << std::endl;
        std::cout << "Barnes-Hut theta: " << params.theta << std::endl;
        std::cout << "MPI ranks: " << size << std::endl;
        std::cout << "GPU acceleration: " << (params.use_gpu ? "enabled" : "disabled") << std::endl;
        std::cout << "========================================\n" << std::endl;
    }

    decompose_domain();
    distribute_particles();
}

void NBodySimulator::run() {
    double sim_start = get_wall_time();

    for (int step = 0; step < params.n_steps; ++step) {
        step_start_time = get_wall_time();

        // Exchange particles with neighbors
        exchange_ghost_particles();

        // Compute forces and integrate
        compute_forces();
        integrate_particles();

        // Output
        output_vtk(step);

        // Load balancing (optional, every N steps)
        if (step % 10 == 0) {
            // balance_load();  // TODO: implement dynamic load balancing
        }

        // Progress reporting
        if (rank == 0 && step % 10 == 0) {
            double step_time = get_wall_time() - step_start_time;
            double particles_per_sec = params.n_particles / step_time;
            std::cout << "Step " << std::setw(5) << step
                      << " | Time: " << std::fixed << std::setprecision(4) << step_time << " s"
                      << " | Throughput: " << std::scientific << std::setprecision(2)
                      << particles_per_sec << " particles/s" << std::endl;
        }
    }

    total_time = get_wall_time() - sim_start;

    collect_statistics();
}

void NBodySimulator::collect_statistics() {
    if (rank == 0) {
        std::cout << "\n=== Performance Statistics ===" << std::endl;
        std::cout << "Total time: " << total_time << " s" << std::endl;
        std::cout << "Compute time: " << compute_time << " s ("
                  << (compute_time/total_time)*100 << "%)" << std::endl;
        std::cout << "Communication time: " << communication_time << " s ("
                  << (communication_time/total_time)*100 << "%)" << std::endl;
        std::cout << "Tree build time: " << tree_build_time << " s ("
                  << (tree_build_time/total_time)*100 << "%)" << std::endl;

        double throughput = total_particle_updates / total_time;
        std::cout << "Average throughput: " << std::scientific << throughput
                  << " particle-updates/s" << std::endl;
        std::cout << "==============================\n" << std::endl;
    }

    // Write performance data to CSV
    if (rank == 0) {
        std::ofstream csv("results/performance_" + std::to_string(size) + "ranks.csv");
        csv << "metric,value\n";
        csv << "total_time," << total_time << "\n";
        csv << "compute_time," << compute_time << "\n";
        csv << "communication_time," << communication_time << "\n";
        csv << "tree_build_time," << tree_build_time << "\n";
        csv << "throughput," << (total_particle_updates / total_time) << "\n";
        csv << "n_particles," << params.n_particles << "\n";
        csv << "n_steps," << params.n_steps << "\n";
        csv << "n_ranks," << size << "\n";
        csv.close();
    }
}

void NBodySimulator::print_statistics() const {
    // Already printed in collect_statistics
}
