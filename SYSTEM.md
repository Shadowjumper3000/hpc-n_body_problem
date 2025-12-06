# HPC System Configuration

## Cluster Information

**Partition:** `cpubase_bycore_b1`  
**Job Scheduler:** SLURM Workload Manager  
**Allocation Mode:** Exclusive node access

## Node Types and Resources

### Available Compute Nodes
- **Total Nodes Used:** 5 nodes (maximum)
- **Cores per Node:** 2 MPI tasks per node
- **CPUs per Task:** 1
- **Total Tasks:** Up to 10 MPI tasks

### Excluded Nodes
The following nodes are excluded from job allocation:
- `node2`
- `node7`
- `node8`
- `gpu-node1`
- `gpu-node2`

### Resource Allocation
- **Strong Scaling Tests:** Fixed problem size (100,000 particles) across 1, 2, 4, 8, and 10 tasks
- **Weak Scaling Tests:** Scaled problem size (10,000 particles per task) across 1, 2, 4, 8, and 10 tasks

## Software Environment

### Modules
Module configuration is loaded from: `../modules.txt`

### Python Environment
- **Python Version:** 3.x
- **Key Dependencies:**
  - `numpy >= 1.24.0` - Core numerical computation
  - `matplotlib >= 3.7.0` - Visualization and plotting
  - `cupy-cuda12x >= 12.0.0` - GPU acceleration (CUDA-enabled)

### MPI Configuration
- **Implementation:** Specified in `modules.txt` (loaded via `source ../modules.txt`)
- **Execution:** `srun` command for parallel task execution

### OpenMP Configuration
```bash
export OMP_NUM_THREADS=1
export OMP_PROC_BIND=close
export OMP_PLACES=cores
```

## GPU Configuration (Optional)

### CUDA Runtime
- **CUDA Version:** 12.x (required for CuPy)
- **GPU Driver:** Compatible with CUDA 12.x toolkit
- **GPU Acceleration:** Optional, falls back to CPU if unavailable

### GPU Nodes
- GPU nodes (`gpu-node1`, `gpu-node2`) are excluded from CPU-only scaling tests
- GPU testing requires separate job submission to GPU partition

## Simulation Parameters

### N-Body Simulation Settings
- **Algorithm:** Barnes-Hut tree code
- **Theta Parameter:** 0.5 (tree approximation threshold)
- **Time Step (dt):** 0.01
- **Steps:** 100
- **Output Frequency:** 20-50 (varies by test)

### Problem Sizes
- **Strong Scaling:** 100,000 particles (fixed)
- **Weak Scaling:** 10,000 particles per task (scales with task count)

## Job Execution

### Time Limits
- **Maximum Wall Time:** 2 hours (02:00:00)

### Output Directory
```bash
../results/
```

### Output Files
- Standard Output: `../results/{test_type}_{job_id}.out`
- Standard Error: `../results/{test_type}_{job_id}.err`
- Simulation Data: `../results/{test_type}_{ntasks}tasks.vtk`

## Performance Testing

### Strong Scaling
Tests parallel efficiency with fixed problem size:
- Measures speedup as processor count increases
- Problem size: 100,000 particles
- Task counts: 1, 2, 4, 8, 10

### Weak Scaling
Tests scalability with proportional problem size:
- Maintains constant work per processor
- Base size: 10,000 particles per task
- Task counts: 1, 2, 4, 8, 10

---

**Document Version:** 1.0  
**Last Updated:** December 6, 2025
