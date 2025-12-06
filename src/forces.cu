#ifdef USE_CUDA

#include "particle.hpp"
#include <cuda_runtime.h>
#include <iostream>

// CUDA error checking macro
#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            std::cerr << "CUDA error at " << __FILE__ << ":" << __LINE__ << ": " \
                      << cudaGetErrorString(err) << std::endl; \
            exit(EXIT_FAILURE); \
        } \
    } while(0)

// Device structure for particles (simpler layout for GPU)
struct ParticleGPU {
    double px, py, pz;     // position
    double vx, vy, vz;     // velocity
    double ax, ay, az;     // acceleration
    double mass;
};

// CUDA kernel for direct N-Body force calculation
// This is O(N²) but highly parallel on GPU
__global__ void compute_forces_kernel(ParticleGPU* particles, int n, double G, double softening) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;

    double ax = 0.0, ay = 0.0, az = 0.0;
    double px = particles[i].px;
    double py = particles[i].py;
    double pz = particles[i].pz;
    double mass_i = particles[i].mass;

    // Calculate force from all other particles
    for (int j = 0; j < n; ++j) {
        if (i == j) continue;

        double dx = particles[j].px - px;
        double dy = particles[j].py - py;
        double dz = particles[j].pz - pz;

        double dist2 = dx*dx + dy*dy + dz*dz + softening*softening;
        double dist = sqrt(dist2);
        double force = G * particles[j].mass / dist2;

        ax += force * dx / dist;
        ay += force * dy / dist;
        az += force * dz / dist;
    }

    particles[i].ax = ax;
    particles[i].ay = ay;
    particles[i].az = az;
}

// Optimized kernel using shared memory for better performance
__global__ void compute_forces_shared_kernel(ParticleGPU* particles, int n, double G, double softening) {
    extern __shared__ ParticleGPU shared_particles[];

    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int tid = threadIdx.x;
    int block_size = blockDim.x;

    double ax = 0.0, ay = 0.0, az = 0.0;

    if (i < n) {
        double px = particles[i].px;
        double py = particles[i].py;
        double pz = particles[i].pz;

        // Process particles in tiles
        for (int tile = 0; tile < (n + block_size - 1) / block_size; ++tile) {
            int j = tile * block_size + tid;

            // Load tile into shared memory
            if (j < n) {
                shared_particles[tid] = particles[j];
            }
            __syncthreads();

            // Compute forces from particles in this tile
            int tile_size = min(block_size, n - tile * block_size);
            for (int k = 0; k < tile_size; ++k) {
                int j_global = tile * block_size + k;
                if (j_global == i) continue;

                double dx = shared_particles[k].px - px;
                double dy = shared_particles[k].py - py;
                double dz = shared_particles[k].pz - pz;

                double dist2 = dx*dx + dy*dy + dz*dz + softening*softening;
                double dist = sqrt(dist2);
                double force = G * shared_particles[k].mass / dist2;

                ax += force * dx / dist;
                ay += force * dy / dist;
                az += force * dz / dist;
            }
            __syncthreads();
        }

        particles[i].ax = ax;
        particles[i].ay = ay;
        particles[i].az = az;
    }
}

// Host function to launch GPU computation
extern "C" void compute_forces_gpu(std::vector<Particle>& particles) {
    int n = particles.size();
    if (n == 0) return;

    // Allocate device memory
    ParticleGPU* d_particles;
    size_t size = n * sizeof(ParticleGPU);
    CUDA_CHECK(cudaMalloc(&d_particles, size));

    // Copy particles to device
    std::vector<ParticleGPU> h_particles(n);
    for (int i = 0; i < n; ++i) {
        h_particles[i].px = particles[i].position.x;
        h_particles[i].py = particles[i].position.y;
        h_particles[i].pz = particles[i].position.z;
        h_particles[i].vx = particles[i].velocity.x;
        h_particles[i].vy = particles[i].velocity.y;
        h_particles[i].vz = particles[i].velocity.z;
        h_particles[i].mass = particles[i].mass;
    }
    CUDA_CHECK(cudaMemcpy(d_particles, h_particles.data(), size, cudaMemcpyHostToDevice));

    // Launch kernel
    int block_size = 256;
    int grid_size = (n + block_size - 1) / block_size;
    size_t shared_mem = block_size * sizeof(ParticleGPU);

    // Use shared memory version for better performance
    compute_forces_shared_kernel<<<grid_size, block_size, shared_mem>>>(
        d_particles, n, G, SOFTENING
    );

    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    // Copy results back
    CUDA_CHECK(cudaMemcpy(h_particles.data(), d_particles, size, cudaMemcpyDeviceToHost));

    for (int i = 0; i < n; ++i) {
        particles[i].acceleration.x = h_particles[i].ax / particles[i].mass;
        particles[i].acceleration.y = h_particles[i].ay / particles[i].mass;
        particles[i].acceleration.z = h_particles[i].az / particles[i].mass;
    }

    // Free device memory
    CUDA_CHECK(cudaFree(d_particles));
}

#endif // USE_CUDA
