#include "nbody.hpp"
#include <iostream>
#include <mpi.h>
#include <getopt.h>
#include <cstdlib>

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --particles N      Number of particles (default: 1000)\n";
    std::cout << "  --steps N          Number of time steps (default: 100)\n";
    std::cout << "  --dt FLOAT         Time step size (default: 0.01)\n";
    std::cout << "  --theta FLOAT      Barnes-Hut opening angle (default: 0.5)\n";
    std::cout << "  --gpu              Enable GPU acceleration\n";
    std::cout << "  --output FILE      Output VTK filename (default: output.vtk)\n";
    std::cout << "  --freq N           Output frequency in steps (default: 10)\n";
    std::cout << "  --domain-size F    Domain size (default: 100.0)\n";
    std::cout << "  --help             Print this help message\n";
}

int main(int argc, char* argv[]) {
    // Initialize MPI
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Parse command-line arguments
    SimulationParams params;

    static struct option long_options[] = {
        {"particles", required_argument, 0, 'p'},
        {"steps", required_argument, 0, 's'},
        {"dt", required_argument, 0, 't'},
        {"theta", required_argument, 0, 'a'},
        {"gpu", no_argument, 0, 'g'},
        {"output", required_argument, 0, 'o'},
        {"freq", required_argument, 0, 'f'},
        {"domain-size", required_argument, 0, 'd'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "p:s:t:a:go:f:d:h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'p':
                params.n_particles = std::atoi(optarg);
                break;
            case 's':
                params.n_steps = std::atoi(optarg);
                break;
            case 't':
                params.dt = std::atof(optarg);
                break;
            case 'a':
                params.theta = std::atof(optarg);
                break;
            case 'g':
                params.use_gpu = true;
                break;
            case 'o':
                params.output_file = optarg;
                break;
            case 'f':
                params.output_frequency = std::atoi(optarg);
                break;
            case 'd':
                params.domain_size = std::atof(optarg);
                break;
            case 'h':
                if (rank == 0) print_usage(argv[0]);
                MPI_Finalize();
                return 0;
            default:
                if (rank == 0) print_usage(argv[0]);
                MPI_Finalize();
                return 1;
        }
    }

    // Validate parameters
    if (params.n_particles <= 0 || params.n_steps <= 0) {
        if (rank == 0) {
            std::cerr << "Error: Invalid parameters\n";
            print_usage(argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    // Run simulation
    try {
        NBodySimulator simulator(params, MPI_COMM_WORLD);
        simulator.initialize();
        simulator.run();
        simulator.print_statistics();
    } catch (const std::exception& e) {
        std::cerr << "Error on rank " << rank << ": " << e.what() << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Finalize();
    return 0;
}
