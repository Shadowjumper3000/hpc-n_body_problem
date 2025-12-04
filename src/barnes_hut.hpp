#ifndef BARNES_HUT_HPP
#define BARNES_HUT_HPP

#include "particle.hpp"
#include <memory>
#include <array>

// Octree node for Barnes-Hut algorithm
class OctreeNode {
public:
    BoundingBox bounds;
    Vec3 center_of_mass;
    double total_mass;
    int particle_count;

    // Children nodes (8 octants)
    std::array<std::unique_ptr<OctreeNode>, 8> children;

    // If leaf node, stores particle index
    int particle_index;
    bool is_leaf;

    OctreeNode(const BoundingBox& box);

    // Insert a particle into the octree
    void insert(const Particle& p, int idx, const std::vector<Particle>& particles);

    // Calculate force on a particle using Barnes-Hut approximation
    Vec3 calculate_force(const Particle& p, double theta) const;

private:
    // Determine which octant a position belongs to
    int get_octant(const Vec3& pos) const;

    // Get bounding box for a specific octant
    BoundingBox get_octant_bounds(int octant) const;

    // Update center of mass when adding particles
    void update_center_of_mass(const Particle& p);
};

// Barnes-Hut tree for efficient force calculation
class BarnesHutTree {
public:
    BarnesHutTree(const std::vector<Particle>& particles, const BoundingBox& domain);

    // Build the octree from particles
    void build(const std::vector<Particle>& particles);

    // Calculate forces on all particles using Barnes-Hut
    void calculate_forces(std::vector<Particle>& particles, double theta);

    // Get tree statistics
    int get_depth() const;
    int get_node_count() const;

private:
    std::unique_ptr<OctreeNode> root;
    BoundingBox domain;

    int calculate_depth(const OctreeNode* node) const;
    int count_nodes(const OctreeNode* node) const;
};

#endif // BARNES_HUT_HPP
