#ifndef GATHERING_PARTICLE_H
#define GATHERING_PARTICLE_H

#include <set>
#include <vector>

#include "gathering/glm_include.hpp"

namespace gathering {

constexpr float RADIUS_PARTICLE = 0.05f;  // TODO optional parameter
constexpr float RADIUS_PARTICLE_SQR = RADIUS_PARTICLE * RADIUS_PARTICLE;
constexpr float RADIUS_PARTICLE_2_SQR = RADIUS_PARTICLE * RADIUS_PARTICLE * 4.0f;
constexpr float VERLET_RADIUS = RADIUS_PARTICLE * 4.0;
constexpr float VERLET_RADIUS_HALF_SQR = VERLET_RADIUS * VERLET_RADIUS / 4.0f;

typedef glm::vec<3, int> vec3i;

struct AABB {
    glm::vec3 min, max;
    AABB() = default;
    AABB(const glm::vec3 min, const glm::vec3 max) : min(min), max(max){};

    bool intersect(const AABB& bb) const;
};

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

struct VerletList {
    std::vector<size_t> objects;
    glm::vec3 center;

    bool isValid(const glm::vec3& pos) {
        glm::vec3 diff = pos - center;
        return glm::dot(diff, diff) <= VERLET_RADIUS_HALF_SQR;
    }
};

class Triangle {
   public:
    glm::vec3 normal;
    glm::vec3 a, b, c;  // vertices of the triangle (used for collision detection)
    AABB bb;

    Triangle() = delete;
    Triangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) : a(a), b(b), c(c) {
        bb = AABB(glm::min(glm::min(a, b), c), glm::max(glm::max(a, b), c));
        normal = glm::normalize(glm::cross(b - a, c - a));
    };
    float intersect(const Ray& ray) const;
};

struct Particle {
    Particle() = delete;
    Particle(const float x, const float y, const float z) : position(glm::vec3(x, y, z)) {}

    bool intersect(const Triangle& triangle) const;
    bool intersect(const Particle& particle) const;
    bool intersect(const AABB& aabb) const { return bb.intersect(aabb); }

    glm::vec3 position = glm::vec3(0.0);
    glm::vec3 old_position = glm::vec3(0.0);
    glm::vec3 velocity = glm::vec3(0.0);
    glm::vec3 new_velocity = glm::vec3(0.0);
    glm::vec3 acceleration = glm::vec3(0.0);
    float mass = 1.0;
    AABB bb;

    vec3i particle_grid_position = vec3i(0);
    std::vector<size_t> close_triangles;
};

}  // namespace gathering

#endif
