#!/bin/bash
#SBATCH --job-name=nbody-cpu-strong
#SBATCH --output=../results/cpu_strong_%j.out
#SBATCH --error=../results/cpu_strong_%j.err
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=8
#SBATCH --time=00:30:00
#SBATCH --mem-per-cpu=1G

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
echo "Nodes: $SLURM_JOB_NUM_NODES"
echo "Tasks per node: $SLURM_NTASKS_PER_NODE"
echo "Total tasks: $SLURM_NTASKS"
echo "CPUs per task: $SLURM_CPUS_PER_TASK"
echo "Working directory: $(pwd)"
echo "========================================="

# Problem size (fixed for strong scaling)
N_PARTICLES=100000
N_STEPS=100
DT=0.01
THETA=0.5
OUTPUT_FREQ=20

# Create results directory
mkdir -p ../results

# Run strong scaling tests (varying number of MPI tasks)
for NTASKS in 1 2 4 8 16 32; do
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
