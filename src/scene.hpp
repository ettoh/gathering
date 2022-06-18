#ifndef GATHERING_SCENE_H
#define GATHERING_SCENE_H

#include <array>
#include <set>
#include <vector>

#include "container.hpp"
#include "gathering/glm_include.hpp"
#include "opengl_primitives.hpp"
#include "particle.hpp"

namespace gathering {

typedef std::pair<size_t, size_t> particle_pair;
constexpr vec3i AMOUNT_CELLS = vec3i(200);  // TODO get rid

class Scene {
   public:
    Scene(const char* file);
    void update(const float dt);
    void addParticles(const int n, const float mass_mean, const float mass_stddev);
    void gridCoordsArea(const AABB& aabb, std::vector<vec3i>& affected_cells) const;
    vec3i gridCoords(const glm::vec3& pos) const;

    std::vector<Particle> particles;
    std::vector<Triangle> triangles;
    glm::vec3 global_force = glm::vec3(0.0f);
    OpenGLPrimitives::Object vessel;
    AABB vessel_bb = AABB(glm::vec3(0), glm::vec3(0));

   private:
    void findCollisionsParticles();
    void findCollisionsTriangles();
    void loadObject(const char* path);
    Grid grid = Grid();
    Array3D<std::vector<size_t>> cells =
        Array3D<std::vector<size_t>>(AMOUNT_CELLS.x, AMOUNT_CELLS.y, AMOUNT_CELLS.z, {});

    Grid particle_grid = Grid();
    std::vector<particle_pair>
        collisions_particle;  // memory for reported collisions between two particles
    std::vector<size_t> collisions_vessel;
    std::vector<size_t> close_particles;

    // properties of the scene
    glm::vec3 max_triangle_size = glm::vec3(0);
    glm::vec3 min_triangle_size = glm::vec3(std::numeric_limits<float>::infinity());
};

}  // namespace gathering

#endif
