#ifndef PARTICLE_HPP
#define PARTICLE_HPP

#include <vector>
#include <cmath>

// Physical constants
constexpr double G = 6.67430e-11;  // Gravitational constant
constexpr double SOFTENING = 1e-9; // Softening parameter to avoid singularities

// 3D Vector structure
struct Vec3 {
    double x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(double s) const { return Vec3(x * s, y * s, z * s); }
    Vec3& operator+=(const Vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }

    double norm() const { return std::sqrt(x*x + y*y + z*z); }
    double norm2() const { return x*x + y*y + z*z; }
};

// Particle structure
struct Particle {
    Vec3 position;
    Vec3 velocity;
    Vec3 acceleration;
    double mass;
    int id;  // Unique particle identifier

    Particle() : mass(1.0), id(0) {}
    Particle(Vec3 pos, Vec3 vel, double m, int particle_id)
        : position(pos), velocity(vel), acceleration(), mass(m), id(particle_id) {}
};

// Bounding box for spatial partitioning
struct BoundingBox {
    Vec3 min;
    Vec3 max;

    BoundingBox() {}
    BoundingBox(Vec3 min_, Vec3 max_) : min(min_), max(max_) {}

    Vec3 center() const {
        return Vec3((min.x + max.x) * 0.5,
                   (min.y + max.y) * 0.5,
                   (min.z + max.z) * 0.5);
    }

    double size() const {
        return std::max({max.x - min.x, max.y - min.y, max.z - min.z});
    }

    bool contains(const Vec3& p) const {
        return p.x >= min.x && p.x <= max.x &&
               p.y >= min.y && p.y <= max.y &&
               p.z >= min.z && p.z <= max.z;
    }
};

// Simulation parameters
struct SimulationParams {
    int n_particles;
    int n_steps;
    double dt;              // Time step
    double theta;           // Barnes-Hut opening angle (0.5 typical)
    bool use_gpu;
    bool save_vtk;
    std::string output_file;
    int output_frequency;   // Save every N steps

    // Domain boundaries
    double domain_size;

    SimulationParams()
        : n_particles(1000), n_steps(100), dt(0.01), theta(0.5),
          use_gpu(false), save_vtk(true), output_file("output.vtk"),
          output_frequency(10), domain_size(100.0) {}
};

#endif // PARTICLE_HPP
