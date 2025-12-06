#!/bin/bash
#SBATCH --job-name=nbody-simple
#SBATCH --output=../results/simple_%j.out
#SBATCH --error=../results/simple_%j.err
#SBATCH --ntasks=1
#SBATCH --time=00:10:00

# Simplest possible test - single task

# Load modules
module purge
source ../env/modules.txt

export OMP_NUM_THREADS=1

echo "========================================="
echo "Simple Single-Task Test"
echo "Job ID: $SLURM_JOB_ID"
echo "========================================="

mkdir -p ../results

# Run with single process
srun ../build/nbody \
    --particles 100 \
    --steps 5 \
    --dt 0.01 \
    --theta 0.5 \
    --freq 1 \
    --output ../results/simple_test.vtk

echo "Test completed!"
