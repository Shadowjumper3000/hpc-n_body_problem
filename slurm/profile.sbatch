#!/bin/bash
#SBATCH --job-name=nbody-profile
#SBATCH --output=../results/profile_%j.out
#SBATCH --error=../results/profile_%j.err
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=4
#SBATCH --gpus-per-node=4
#SBATCH --cpus-per-task=8
#SBATCH --time=00:30:00
#SBATCH --partition=gpu

# Load modules
module purge
source ../env/modules.txt

# Load profiling tools
# module load likwid  # Uncomment if available
# module load nsight-systems  # For GPU profiling

# Set environment
export OMP_NUM_THREADS=${SLURM_CPUS_PER_TASK}

echo "========================================="
echo "Profiling N-Body Simulation"
echo "Job ID: $SLURM_JOB_ID"
echo "========================================="

# Create results directory
mkdir -p ../results/profiling

N_PARTICLES=100000
N_STEPS=50
DT=0.01
THETA=0.5

# CPU Profiling with perf (if available)
echo ""
echo "CPU Profiling with perf..."
srun --nodes=1 --ntasks=4 --gpus-per-task=0 \
    perf stat -e cycles,instructions,cache-references,cache-misses,branches,branch-misses \
    ../build/nbody \
    --particles $N_PARTICLES \
    --steps $N_STEPS \
    --dt $DT \
    --theta $THETA \
    --output ../results/profiling/cpu_profile.vtk \
    2>&1 | tee ../results/profiling/perf_cpu.txt

# GPU Profiling with Nsight Systems (if available)
echo ""
echo "GPU Profiling with Nsight Systems..."
# srun --nodes=1 --ntasks=2 --gpus-per-task=1 \
#     nsys profile -o ../results/profiling/gpu_profile \
#     --stats=true --force-overwrite=true \
#     ../build/nbody \
#     --particles $N_PARTICLES \
#     --steps $N_STEPS \
#     --dt $DT \
#     --theta $THETA \
#     --gpu \
#     --output ../results/profiling/gpu_profile.vtk

# Memory bandwidth profiling (if LIKWID available)
echo ""
echo "Memory bandwidth profiling..."
# srun --nodes=1 --ntasks=4 \
#     likwid-perfctr -C 0-15 -g MEM \
#     ../build/nbody \
#     --particles $N_PARTICLES \
#     --steps $N_STEPS \
#     --dt $DT \
#     --theta $THETA \
#     --output ../results/profiling/mem_profile.vtk \
#     2>&1 | tee ../results/profiling/likwid_mem.txt

echo ""
echo "Profiling completed!"
echo "Results saved in ../results/profiling/"
