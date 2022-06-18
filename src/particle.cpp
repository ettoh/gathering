#include "particle.hpp"

namespace gathering {

using glm::vec3;

float Triangle::intersect(const Ray& ray) const {
    // plane's normal
    vec3 ab = b - a;
    vec3 ac = c - a;
    vec3 N = glm::cross(ab, ac);

    // ray and plane are parallel?
    float NdotRay = glm::dot(N, ray.direction);
    if (fabs(NdotRay) < 0.0001f) return -1;
    float d = glm::dot(-N, a);

    // intersection at t
    float t = -(glm::dot(N, ray.origin) + d) / NdotRay;
    if (t < 0) return -1.f;

    // intersection point
    vec3 P = ray.origin + t * ray.direction;
    vec3 perp;  // perpendicular to triangle plane

    // AB
    vec3 edge0 = b - a;
    vec3 vp0 = P - a;
    perp = glm::cross(edge0, vp0);
    if (glm::dot(N, perp) < 0) return -1.f;

    // BC
    vec3 edge1 = c - b;
    vec3 vp1 = P - b;
    perp = glm::cross(edge1, vp1);
    if (glm::dot(N, perp) < 0) return -1.f;

    // CA
    vec3 edge2 = a - c;
    vec3 vp2 = P - c;
    perp = glm::cross(edge2, vp2);
    if (glm::dot(N, perp) < 0) return -1.f;

    return t;
}

// ------------------------------------------------------------------------------------------------

bool AABB::intersect(const AABB& bb) const {
    return (min.x <= bb.max.x && max.x >= bb.min.x) &&
           (min.y <= bb.max.y && max.y >= bb.min.y) &&
           (min.z <= bb.max.z && max.z >= bb.min.z);
}

// ------------------------------------------------------------------------------------------------

bool Particle::intersect(const Particle& particle) const {
    glm::vec3 diff = particle.position - position;
    return glm::dot(diff, diff) < RADIUS_PARTICLE_2_SQR;
}

// ------------------------------------------------------------------------------------------------

bool Particle::intersect(const Triangle& t) const {
    // 1. sphere VS plane
    vec3 v = position - t.a;
    float distance = glm::abs(glm::dot(v, t.normal));
    if (distance > RADIUS_PARTICLE) {
        return false;
    }

    // 2. sphere VS triangle vertices
    glm::vec3 da = position - t.a;
    if (glm::dot(da, da) <= RADIUS_PARTICLE_SQR) return true;
    glm::vec3 db = position - t.b;
    if (glm::dot(db, db) <= RADIUS_PARTICLE_SQR) return true;
    glm::vec3 dc = position - t.c;
    if (glm::dot(dc, dc) <= RADIUS_PARTICLE_SQR) return true;

    // 3. sphere VS triangle edges
    // AB
    vec3 ab = t.b - t.a;
    vec3 as = position - t.a;
    float scale = glm::dot(as, ab) / glm::dot(ab, ab);
    if (scale > 0.f && scale < 1.f) {
        glm::vec3 tmp = as - ab * scale;
        if (glm::dot(tmp, tmp) <= RADIUS_PARTICLE_SQR) return true;
    }

    // AC
    vec3 ac = t.c - t.a;
    as = position - t.a;
    scale = glm::dot(as, ac) / glm::dot(ac, ac);
    if (scale > 0.f && scale < 1.f) {
        glm::vec3 tmp = as - ac * scale;
        if (glm::dot(tmp, tmp) <= RADIUS_PARTICLE_SQR) return true;
    }

    // CB
    vec3 cb = t.b - t.c;
    as = position - t.a;
    scale = glm::dot(as, cb) / glm::dot(cb, cb);
    if (scale > 0.f && scale < 1.f) {
        glm::vec3 tmp = as - cb * scale;
        if (glm::dot(tmp, tmp) <= RADIUS_PARTICLE_SQR) return true;
    }

    // 4. sphere VS triangle inside
    vec3 p = position + distance * t.normal;  // center of sphere projected to triangle plane
    float m1 = ((t.b.y - t.c.y) * (p.x - t.c.x) + (t.c.x - t.b.x) * (p.y - t.c.y)) /
               ((t.b.y - t.c.y) * (t.a.x - t.c.x) + (t.c.x - t.b.x) * (t.a.y - t.c.y));
    if (m1 < 0 || m1 > 1) return false;
    float m2 = ((t.c.y - t.a.y) * (p.x - t.c.x) + (t.a.x - t.c.x) * (p.y - t.c.y)) /
               ((t.b.y - t.c.y) * (t.a.x - t.c.x) + (t.c.x - t.b.x) * (t.a.y - t.c.y));
    if (m2 < 0) return false;
    if (m1 + m2 > 1) return false;
    return true;
}

}  // namespace gathering
