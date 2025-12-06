#ifndef NBODY_HPP
#define NBODY_HPP

#include "particle.hpp"
#include "barnes_hut.hpp"
#include <mpi.h>
#include <vector>
#include <string>

// MPI-enabled N-Body simulator
class NBodySimulator {
public:
    NBodySimulator(const SimulationParams& params, MPI_Comm comm);
    ~NBodySimulator();

    // Initialize particles (only root generates, then distributes)
    void initialize();

    // Run the simulation
    void run();

    // Get simulation statistics
    void print_statistics() const;

private:
    SimulationParams params;
    std::vector<Particle> local_particles;  // Particles owned by this rank
    std::vector<Particle> ghost_particles;  // Halo particles from neighbors

    // MPI communication
    MPI_Comm comm;
    int rank;
    int size;

    // Domain decomposition
    BoundingBox local_domain;
    BoundingBox global_domain;

    // Performance metrics
    double total_time;
    double compute_time;
    double communication_time;
    double tree_build_time;
    long long total_particle_updates;

    // Timers
    double start_time;
    double step_start_time;

    // Helper functions
    void generate_initial_conditions(std::vector<Particle>& particles);
    void distribute_particles();
    void exchange_ghost_particles();
    void compute_forces();
    void integrate_particles();
    void output_vtk(int step);
    void collect_statistics();
    BoundingBox compute_global_bounds();
    void decompose_domain();

    // Load balancing
    void balance_load();

    // GPU acceleration (if enabled)
#ifdef USE_CUDA
    void compute_forces_gpu();
#endif
};

// Utility functions
void setup_mpi_particle_type(MPI_Datatype* particle_type);
double get_wall_time();

#endif // NBODY_HPP
