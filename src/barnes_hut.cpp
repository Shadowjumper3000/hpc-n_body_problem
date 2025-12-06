#include "barnes_hut.hpp"
#include <cmath>
#include <algorithm>

// OctreeNode implementation
OctreeNode::OctreeNode(const BoundingBox& box)
    : bounds(box), center_of_mass(), total_mass(0.0),
      particle_count(0), particle_index(-1), is_leaf(true) {
    for (auto& child : children) {
        child = nullptr;
    }
}

void OctreeNode::update_center_of_mass(const Particle& p) {
    if (particle_count == 0) {
        center_of_mass = p.position;
        total_mass = p.mass;
    } else {
        // Weighted average
        double total = total_mass + p.mass;
        center_of_mass.x = (center_of_mass.x * total_mass + p.position.x * p.mass) / total;
        center_of_mass.y = (center_of_mass.y * total_mass + p.position.y * p.mass) / total;
        center_of_mass.z = (center_of_mass.z * total_mass + p.position.z * p.mass) / total;
        total_mass = total;
    }
    particle_count++;
}

int OctreeNode::get_octant(const Vec3& pos) const {
    Vec3 center = bounds.center();
    int octant = 0;
    if (pos.x >= center.x) octant |= 4;
    if (pos.y >= center.y) octant |= 2;
    if (pos.z >= center.z) octant |= 1;
    return octant;
}

BoundingBox OctreeNode::get_octant_bounds(int octant) const {
    Vec3 center = bounds.center();
    Vec3 min = bounds.min;
    Vec3 max = bounds.max;

    BoundingBox octant_bounds;

    // X dimension
    if (octant & 4) {
        octant_bounds.min.x = center.x;
        octant_bounds.max.x = max.x;
    } else {
        octant_bounds.min.x = min.x;
        octant_bounds.max.x = center.x;
    }

    // Y dimension
    if (octant & 2) {
        octant_bounds.min.y = center.y;
        octant_bounds.max.y = max.y;
    } else {
        octant_bounds.min.y = min.y;
        octant_bounds.max.y = center.y;
    }

    // Z dimension
    if (octant & 1) {
        octant_bounds.min.z = center.z;
        octant_bounds.max.z = max.z;
    } else {
        octant_bounds.min.z = min.z;
        octant_bounds.max.z = center.z;
    }

    return octant_bounds;
}

void OctreeNode::insert(const Particle& p, int idx, const std::vector<Particle>& particles) {
    // Update this node's center of mass
    update_center_of_mass(p);

    // If this is an empty leaf, make it contain this particle
    if (is_leaf && particle_index == -1) {
        particle_index = idx;
        return;
    }

    // If this leaf already has a particle, subdivide
    if (is_leaf && particle_index != -1) {
        is_leaf = false;
        int old_idx = particle_index;
        particle_index = -1;

        // Insert the old particle into appropriate child
        int octant = get_octant(particles[old_idx].position);
        if (!children[octant]) {
            children[octant] = std::make_unique<OctreeNode>(get_octant_bounds(octant));
        }
        children[octant]->insert(particles[old_idx], old_idx, particles);
    }

    // Insert new particle into appropriate child
    int octant = get_octant(p.position);
    if (!children[octant]) {
        children[octant] = std::make_unique<OctreeNode>(get_octant_bounds(octant));
    }
    children[octant]->insert(p, idx, particles);
}

Vec3 OctreeNode::calculate_force(const Particle& p, double theta) const {
    // If this is an external node (single particle)
    if (is_leaf && particle_index != -1) {
        Vec3 diff = center_of_mass - p.position;
        double dist2 = diff.norm2() + SOFTENING * SOFTENING;

        // Don't calculate force on itself
        if (dist2 < 1e-10) return Vec3();

        double dist = std::sqrt(dist2);
        double force_magnitude = G * p.mass * total_mass / dist2;

        return diff * (force_magnitude / dist);
    }

    // Empty node
    if (particle_count == 0) return Vec3();

    // Calculate distance to center of mass
    Vec3 diff = center_of_mass - p.position;
    double dist = diff.norm();

    // If node is sufficiently far away, treat as single body (Barnes-Hut approximation)
    double cell_size = bounds.size();
    if (cell_size / dist < theta) {
        double dist2 = dist * dist + SOFTENING * SOFTENING;
        double force_magnitude = G * p.mass * total_mass / dist2;
        return diff * (force_magnitude / dist);
    }

    // Otherwise, recursively calculate force from children
    Vec3 total_force;
    for (const auto& child : children) {
        if (child) {
            total_force += child->calculate_force(p, theta);
        }
    }

    return total_force;
}

// BarnesHutTree implementation
BarnesHutTree::BarnesHutTree(const std::vector<Particle>& particles, const BoundingBox& domain_box)
    : domain(domain_box) {
    build(particles);
}

void BarnesHutTree::build(const std::vector<Particle>& particles) {
    root = std::make_unique<OctreeNode>(domain);

    for (size_t i = 0; i < particles.size(); ++i) {
        if (domain.contains(particles[i].position)) {
            root->insert(particles[i], static_cast<int>(i), particles);
        }
    }
}

void BarnesHutTree::calculate_forces(std::vector<Particle>& particles, double theta) {
    #pragma omp parallel for
    for (size_t i = 0; i < particles.size(); ++i) {
        Vec3 force = root->calculate_force(particles[i], theta);
        particles[i].acceleration = force * (1.0 / particles[i].mass);
    }
}

int BarnesHutTree::calculate_depth(const OctreeNode* node) const {
    if (!node || node->is_leaf) return 0;

    int max_depth = 0;
    for (const auto& child : node->children) {
        if (child) {
            max_depth = std::max(max_depth, calculate_depth(child.get()));
        }
    }
    return max_depth + 1;
}

int BarnesHutTree::count_nodes(const OctreeNode* node) const {
    if (!node) return 0;

    int count = 1;
    for (const auto& child : node->children) {
        count += count_nodes(child.get());
    }
    return count;
}

int BarnesHutTree::get_depth() const {
    return calculate_depth(root.get());
}

int BarnesHutTree::get_node_count() const {
    return count_nodes(root.get());
}
