#!/bin/bash
#SBATCH --job-name=nbody-cpu-strong
#SBATCH --output=../results/cpu_strong_%j.out
#SBATCH --error=../results/cpu_strong_%j.err
#SBATCH --partition=cpubase_bycore_b1
#SBATCH --ntasks=18
#SBATCH --cpus-per-task=1
#SBATCH --exclude=node2,node7,node8
#SBATCH --time=00:30:00

# Load modules
source ../modules.txt

# OpenMP settings
export OMP_NUM_THREADS=1
export OMP_PROC_BIND=close
export OMP_PLACES=cores

# Print job info
echo "========================================="
echo "Job ID: $SLURM_JOB_ID"
echo "Job name: $SLURM_JOB_NAME"
echo "Working directory: $(pwd)"
echo "========================================="

# Problem size
N_PARTICLES=100000
N_STEPS=100
DT=0.01
THETA=0.5
OUTPUT_FREQ=20

mkdir -p ../results

# Use the number of tasks allocated by SLURM
TOTAL_CPUS=${SLURM_NTASKS:-18}
echo "Using $TOTAL_CPUS total CPUs/tasks"

# Strong scaling loop: powers of 2 up to TOTAL_CPUS for efficiency
# This tests 1, 2, 4, 8, 16 tasks (much faster than testing every single value)
for NTASKS in 1 2 4 8 16; do
    # Skip if NTASKS exceeds available resources
    if [ $NTASKS -gt $TOTAL_CPUS ]; then
        echo "Skipping $NTASKS tasks (exceeds available $TOTAL_CPUS)"
        continue
    fi
    echo ""
    echo "Running with $NTASKS MPI tasks..."

    srun --ntasks=$NTASKS ../build/nbody \
        --particles $N_PARTICLES \
        --steps $N_STEPS \
        --dt $DT \
        --theta $THETA \
        --freq $OUTPUT_FREQ \
        --output ../results/strong_${NTASKS}tasks.vtk

    echo "Completed $NTASKS tasks"
done

echo ""
echo "Strong scaling test completed!"
echo "Results saved in ../results/"
