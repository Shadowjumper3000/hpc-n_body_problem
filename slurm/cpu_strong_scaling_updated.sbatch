#!/bin/bash
#SBATCH --job-name=nbody-cpu-strong
#SBATCH --output=../results/cpu_strong_%j.out
#SBATCH --error=../results/cpu_strong_%j.err
#SBATCH --cpus-per-task=1
#SBATCH --gres=gpu:0
#SBATCH --exclude=node2,node7,node8
#SBATCH --time=00:30:00

# Load modules
module purge
source ../env/modules.txt

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

# Dynamically get the total number of CPUs available on all up nodes
TOTAL_CPUS=$(sinfo -h -o "%C" -N -e --partition=cpubase_bycore_b1 | awk -F '/' '{used=$2; idle=$3; other=$4; avail=$5} END{print $2+$3+$4+$5}')
# Fallback if the above fails, you can manually set TOTAL_CPUS
if [ -z "$TOTAL_CPUS" ]; then
    TOTAL_CPUS=18
fi
echo "Detected total available CPUs: $TOTAL_CPUS"

# Strong scaling loop: 1 → TOTAL_CPUS
for NTASKS in $(seq 1 $TOTAL_CPUS); do
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
