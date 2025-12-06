#!/bin/bash
#SBATCH --job-name=nbody-test
#SBATCH --output=../results/test_%j.out
#SBATCH --error=../results/test_%j.err
#SBATCH --ntasks=4
#SBATCH --time=00:10:00
#SBATCH --mem-per-cpu=1G

# Minimal test run - removed cpus-per-task and explicit nodes request

# Load modules
module purge
source ../env/modules.txt

export OMP_NUM_THREADS=1

echo "========================================="
echo "Minimal Test Run"
echo "Job ID: $SLURM_JOB_ID"
echo "Nodes: $SLURM_JOB_NUM_NODES"
echo "Tasks: $SLURM_NTASKS"
echo "========================================="

mkdir -p ../results

# Quick test with small problem
srun ../build/nbody \
    --particles 1000 \
    --steps 10 \
    --dt 0.01 \
    --theta 0.5 \
    --freq 5 \
    --output ../results/test.vtk

echo ""
echo "Test completed successfully!"
