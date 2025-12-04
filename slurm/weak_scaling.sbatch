#!/bin/bash
#SBATCH --job-name=nbody-weak
#SBATCH --output=../results/weak_%j.out
#SBATCH --error=../results/weak_%j.err
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=16
#SBATCH --cpus-per-task=1
#SBATCH --time=01:00:00
#SBATCH --partition=cpu

# Load modules
module purge
source ../env/modules.txt

# Set OpenMP threads
export OMP_NUM_THREADS=${SLURM_CPUS_PER_TASK}
export OMP_PROC_BIND=close
export OMP_PLACES=cores

# Print job information
echo "========================================="
echo "Job ID: $SLURM_JOB_ID"
echo "Job name: $SLURM_JOB_NAME"
echo "========================================="

# Base problem size per node
BASE_PARTICLES=10000
N_STEPS=100
DT=0.01
THETA=0.5
OUTPUT_FREQ=50

# Create results directory
mkdir -p ../results

# Run weak scaling tests (scale problem size with nodes)
for NODES in 1 2 4 8; do
    echo ""
    echo "Running weak scaling with $NODES nodes..."

    NTASKS=$((NODES * 16))
    N_PARTICLES=$((BASE_PARTICLES * NODES))

    echo "Total particles: $N_PARTICLES"

    srun --nodes=$NODES --ntasks=$NTASKS ../build/nbody \
        --particles $N_PARTICLES \
        --steps $N_STEPS \
        --dt $DT \
        --theta $THETA \
        --freq $OUTPUT_FREQ \
        --output ../results/weak_${NODES}nodes.vtk

    echo "Completed $NODES nodes"
done

echo ""
echo "Weak scaling test completed!"
echo "Results saved in ../results/"
