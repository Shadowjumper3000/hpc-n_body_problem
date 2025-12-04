# N-Body Simulation with Barnes-Hut Algorithm

High-Performance Computing Project - Distributed N-Body Simulation

## Team Members
- DAVID ALAN HOERZ
- DAVID VELASCO HERRUZO
- HUGO KOTÚC
- ILINCA CORBU
- JOSÉ MARÍA LARIOS MADRID
- OMAR HAYTHAM ATTA ISSA
- VLAD SOIMU

## Project Overview

This project implements a high-performance N-Body gravitational simulation using the Barnes-Hut algorithm for computational efficiency (O(n log n) instead of O(n²)). The implementation leverages:

- **C++17** for core implementation
- **MPI** for distributed memory parallelization across nodes
- **CUDA** for GPU acceleration of force calculations
- **OpenMP** for shared memory parallelization
- **VTK** for scientific visualization
- **Barnes-Hut Octree** for hierarchical force approximation

## Repository Structure

```
.
├── src/              # Source code
│   ├── nbody.cpp     # Main N-Body simulation
│   ├── nbody.hpp     # Header file
│   ├── barnes_hut.cpp # Barnes-Hut octree implementation
│   ├── barnes_hut.hpp
│   ├── particle.hpp  # Particle data structures
│   ├── forces.cu     # CUDA kernel for force calculations
│   └── visualization.cpp # VTK visualization output
├── env/              # Environment setup
│   ├── project.def   # Apptainer definition
│   └── modules.txt   # Module load commands
├── slurm/            # SLURM submission scripts
│   ├── cpu_strong_scaling.sbatch
│   ├── gpu_strong_scaling.sbatch
│   ├── weak_scaling.sbatch
│   └── profile.sbatch
├── data/             # Input data and generation scripts
│   ├── generate_initial_conditions.py
│   └── README.md
├── results/          # Output data, plots, and logs
│   └── README.md
├── docs/             # Documentation
│   ├── paper.pdf     # 4-6 page technical paper
│   ├── proposal.pdf  # EuroHPC Development Access Proposal
│   ├── slides.pdf    # 5-slide pitch
│   └── reproduce.md  # Reproducibility guide
├── CMakeLists.txt    # Build configuration
└── README.md         # This file
```

## Quick Start

### 1. Build the Project

```bash
# Load required modules
source env/modules.txt

# Build
mkdir build && cd build
cmake ..
make -j8
```

### 2. Run Locally (Single Node)

```bash
# CPU-only (MPI + OpenMP)
mpirun -np 4 ./nbody --particles 10000 --steps 100 --output ../results/test.vtk

# GPU-accelerated
mpirun -np 2 ./nbody --particles 100000 --steps 100 --gpu --output ../results/test.vtk
```

### 3. Submit to SLURM Cluster

```bash
# Strong scaling test (2-8 nodes)
sbatch slurm/gpu_strong_scaling.sbatch

# Weak scaling test
sbatch slurm/weak_scaling.sbatch

# Check job status
squeue -u $USER
```

## Performance Goals

- **Strong Scaling**: Scale from 1 to 8 GPU nodes with >60% parallel efficiency
- **Weak Scaling**: Maintain constant time-per-step as problem size and nodes increase proportionally
- **Throughput**: Target >1M particle-updates/second on 8 GPU nodes
- **Optimization**: Barnes-Hut (θ=0.5) reduces complexity from O(n²) to O(n log n)

## Key Features

1. **Barnes-Hut Algorithm**: Hierarchical octree for fast force approximation
2. **MPI Domain Decomposition**: Spatial partitioning across nodes with ghost zones
3. **GPU Acceleration**: CUDA kernels for parallel force calculation
4. **Load Balancing**: Dynamic redistribution based on particle density
5. **Visualization**: VTK output for ParaView rendering
6. **Checkpointing**: HDF5-based state saving for fault tolerance

## Building Blocks

### Physics
- Gravitational N-Body problem with softening parameter
- Leapfrog integration for time evolution
- Periodic boundary conditions (optional)

### Parallelization Strategy
- Spatial domain decomposition for MPI ranks
- Octree construction on each rank
- All-to-all communication for halo exchange
- GPU offload for force kernel

### Performance Metrics
- Wall-clock time per timestep
- Particle-updates per second
- Parallel efficiency (strong/weak)
- GPU utilization
- Communication overhead

## Dependencies

- **Required**:
  - CMake ≥ 3.18
  - C++17 compiler (GCC 9+, Clang 10+)
  - MPI (OpenMPI or MPICH)
  - CUDA Toolkit ≥ 11.0 (for GPU version)

- **Optional**:
  - VTK ≥ 9.0 (for visualization)
  - HDF5 (for checkpointing)
  - LIKWID/PAPI (for profiling)

## Results Preview

See `results/` directory for:
- Scaling plots (strong/weak)
- Profiling data (Nsight Systems, perf)
- Visualization snapshots
- Performance logs

## Documentation

- **Technical Paper**: `docs/paper.pdf` - Complete methodology and results
- **EuroHPC Proposal**: `docs/proposal.pdf` - Resource request for large-scale runs
- **Reproduction Guide**: `docs/reproduce.md` - Step-by-step instructions
- **Pitch Slides**: `docs/slides.pdf` - 5-minute presentation

## References

- Barnes, J., & Hut, P. (1986). "A hierarchical O(N log N) force-calculation algorithm." Nature, 324(6096), 446-449.
- Hernquist, L. (1987). "Performance characteristics of tree codes." The Astrophysical Journal Supplement Series, 64, 715-734.

## License

See LICENSE file for details.
