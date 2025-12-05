#!/bin/bash
#SBATCH --job-name=nbody-weak
#SBATCH --output=../results/weak_%j.out
#SBATCH --error=../results/weak_%j.err
#SBATCH --partition=cpubase_bycore_b1
#SBATCH --nodes=1-5
#SBATCH --ntasks=10
#SBATCH --ntasks-per-node=2
#SBATCH --cpus-per-task=1
#SBATCH --exclude=node2,node7,node8,gpu-node1,gpu-node2
#SBATCH --exclusive
#SBATCH --time=02:00:00

# Load modules
source ../modules.txt

# OpenMP settings
export OMP_NUM_THREADS=1
export OMP_PROC_BIND=close
export OMP_PLACES=cores

# Print job information
echo "========================================="
echo "Job ID: $SLURM_JOB_ID"
echo "Job name: $SLURM_JOB_NAME"
echo "Working directory: $(pwd)"
echo "========================================="

# Base problem size per task (weak scaling maintains work per processor)
BASE_PARTICLES=10000
N_STEPS=100
DT=0.01
THETA=0.5
OUTPUT_FREQ=50

mkdir -p ../results

# Use the number of tasks allocated by SLURM
TOTAL_CPUS=${SLURM_NTASKS:-10}
echo "Using $TOTAL_CPUS total CPUs/tasks across $SLURM_JOB_NUM_NODES nodes"

# Weak scaling loop: powers of 2 up to TOTAL_CPUS
# Each task gets BASE_PARTICLES, so total particles scales with tasks
for NTASKS in 1 2 4 8 10; do
    # Skip if NTASKS exceeds available resources
    if [ $NTASKS -gt $TOTAL_CPUS ]; then
        echo "Skipping $NTASKS tasks (exceeds available $TOTAL_CPUS)"
        continue
    fi

    echo ""
    echo "Running weak scaling with $NTASKS tasks..."

    # Scale particles with number of tasks
    N_PARTICLES=$((BASE_PARTICLES * NTASKS))

    echo "Total particles: $N_PARTICLES ($BASE_PARTICLES per task)"
    echo "Tasks: $NTASKS"

    srun --ntasks=$NTASKS ../build/nbody \
        --particles $N_PARTICLES \
        --steps $N_STEPS \
        --dt $DT \
        --theta $THETA \
        --freq $OUTPUT_FREQ \
        --output ../results/weak_${NTASKS}tasks.vtk

    echo "Completed $NTASKS tasks"
done

echo ""
echo "Weak scaling test completed!"
echo "Results saved in ../results/"
