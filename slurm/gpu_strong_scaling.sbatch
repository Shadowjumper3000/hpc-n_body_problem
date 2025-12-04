#!/bin/bash
#SBATCH --job-name=nbody-gpu-strong
#SBATCH --output=../results/gpu_strong_%j.out
#SBATCH --error=../results/gpu_strong_%j.err
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=4
#SBATCH --gpus-per-node=4
#SBATCH --cpus-per-task=8
#SBATCH --time=01:00:00
#SBATCH --partition=gpu

# Load modules
module purge
source ../env/modules.txt

# Set environment
export OMP_NUM_THREADS=${SLURM_CPUS_PER_TASK}
export CUDA_VISIBLE_DEVICES=0,1,2,3

# Print job information
echo "========================================="
echo "Job ID: $SLURM_JOB_ID"
echo "Job name: $SLURM_JOB_NAME"
echo "Nodes: $SLURM_JOB_NUM_NODES"
echo "GPUs per node: $SLURM_GPUS_PER_NODE"
echo "Tasks per node: $SLURM_NTASKS_PER_NODE"
echo "Total tasks: $SLURM_NTASKS"
echo "CPUs per task: $SLURM_CPUS_PER_TASK"
echo "========================================="

# Print GPU info
nvidia-smi

# Problem size (fixed for strong scaling)
N_PARTICLES=500000
N_STEPS=100
DT=0.01
THETA=0.5
OUTPUT_FREQ=20

# Create results directory
mkdir -p ../results

# Run strong scaling tests with GPUs
for NODES in 1 2 4 8; do
    echo ""
    echo "Running with $NODES GPU nodes..."

    NTASKS=$((NODES * 4))  # 4 GPUs per node

    srun --nodes=$NODES --ntasks=$NTASKS --gpus-per-task=1 \
        ../build/nbody \
        --particles $N_PARTICLES \
        --steps $N_STEPS \
        --dt $DT \
        --theta $THETA \
        --gpu \
        --freq $OUTPUT_FREQ \
        --output ../results/gpu_strong_${NODES}nodes.vtk

    echo "Completed $NODES GPU nodes"
done

echo ""
echo "GPU strong scaling test completed!"
echo "Results saved in ../results/"
