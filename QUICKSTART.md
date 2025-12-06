# Quick Start Guide

Get up and running with the N-Body simulation in 5 minutes!

## Prerequisites

- Access to HPC cluster with Slurm
- MPI installed (OpenMPI or MPICH)
- C++17 compiler (GCC 9+ or Clang 10+)
- CMake 3.18+

## 1. Clone and Build (2 minutes)

```bash
# Clone repository
git clone https://github.com/your-team/hpc-n_body_problem.git
cd hpc-n_body_problem

# Load required modules
source env/modules.txt

# Build (CPU-only version)
./build.sh cpu

# Or GPU version (if CUDA available)
# ./build.sh gpu
```

## 2. Test Locally (1 minute)

```bash
# Quick test with 1000 particles
mpirun -np 4 ./build/nbody \
    --particles 1000 \
    --steps 10 \
    --output results/test.vtk

# Check output
ls -lh results/test_*.vtk
```

Expected output:
```
=== N-Body Simulation with Barnes-Hut ===
Particles: 1000
Steps: 10
...
Step 0 | Time: 0.0234 s | Throughput: 4.27e+04 particles/s
...
```

## 3. Submit to Cluster (2 minutes)

```bash
# Create results directory
mkdir -p results

# Submit a single-node test job
cd slurm
sbatch single_node_test.sbatch

# Check job status
squeue -u $USER

# View output (after job completes)
cat ../results/test_*.out
```

## 4. Visualize Results (Optional)

```bash
# Generate plots from performance data
python3 scripts/plot_scaling.py results/

# View in ParaView (download VTK files first)
# paraview results/test_0.vtk
```

## Next Steps

### Run Scaling Tests

**Strong scaling** (fixed problem size, more nodes):
```bash
cd slurm
sbatch cpu_strong_scaling.sbatch
```

**Weak scaling** (problem size grows with nodes):
```bash
cd slurm
sbatch weak_scaling.sbatch
```

**GPU acceleration** (if available):
```bash
cd slurm
sbatch gpu_strong_scaling.sbatch
```

### Performance Profiling

```bash
cd slurm
sbatch profile.sbatch
```

### Analyze Results

```bash
# Generate scaling plots
python3 scripts/plot_scaling.py results/

# View performance summary
cat results/plots/scaling_summary.csv

# Visualize particle data
python3 scripts/visualize_vtk.py results/test_0.vtk
```

## Common Parameters

| Parameter | Description | Default | Example |
|-----------|-------------|---------|---------|
| `--particles N` | Number of particles | 1000 | `--particles 100000` |
| `--steps N` | Time steps to simulate | 100 | `--steps 500` |
| `--dt FLOAT` | Time step size | 0.01 | `--dt 0.005` |
| `--theta FLOAT` | Barnes-Hut accuracy | 0.5 | `--theta 0.3` |
| `--gpu` | Enable GPU acceleration | off | `--gpu` |
| `--output FILE` | VTK output file | output.vtk | `--output sim.vtk` |
| `--freq N` | Output every N steps | 10 | `--freq 20` |

## Troubleshooting

**Build fails**:
```bash
# Make sure modules are loaded
source env/modules.txt
module list

# Clean and rebuild
./build.sh clean
./build.sh cpu
```

**MPI warning about vader**:
```bash
export OMPI_MCA_btl_vader_single_copy_mechanism=none
```

**Job fails immediately**:
```bash
# Check available partitions
sinfo

# Verify partition name in sbatch scripts matches
```

**Need help?**
- Read full documentation: `docs/reproduce.md`
- Check system requirements: `SYSTEM.md`
- Open issue on GitHub

## Performance Expectations

| System | Particles | Nodes | Time | Throughput |
|--------|-----------|-------|------|------------|
| Single CPU node | 10K | 1 | ~5s | ~20K p/s |
| 4 CPU nodes | 100K | 4 | ~30s | ~330K p/s |
| 8 CPU nodes | 100K | 8 | ~15s | ~665K p/s |
| 1 GPU | 100K | 1 | ~1s | ~10M p/s |

**Note**: Actual performance depends on hardware, problem size, and configuration.

## Files Structure

```
hpc-n_body_problem/
├── README.md           ← Start here for overview
├── QUICKSTART.md       ← This file
├── build.sh            ← Build script
├── CMakeLists.txt      ← Build configuration
├── src/                ← Source code
├── slurm/              ← Job submission scripts
├── env/                ← Environment setup
├── docs/               ← Detailed documentation
├── scripts/            ← Analysis utilities
└── results/            ← Output directory
```

## What's Next?

1. ✅ **Built and tested** locally
2. 📊 **Run scaling experiments** on cluster
3. 📈 **Analyze performance** with provided scripts
4. 📝 **Write paper** using templates in `docs/`
5. 🎓 **Present results** (see `docs/SLIDES_OUTLINE.md`)

Happy simulating! 🚀
