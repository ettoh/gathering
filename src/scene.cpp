#include "scene.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

#include "meta.hpp"

namespace gathering {

using glm::vec3;

Scene::Scene(const char* file) {
    loadObject(file);
    close_particles.reserve(32);

    vec3i resolution = (vessel_bb.max - vessel_bb.min) / glm::vec3(RADIUS_PARTICLE * 6);
    particle_grid = Grid(resolution, vessel_bb);
}

// TODO improve
void Scene::addParticles(const int n, const float mass_mean, const float mass_stddev) {
    std::default_random_engine generator;
    std::normal_distribution<float> distribution(mass_mean, mass_stddev);
    Array3D<bool> inside_cells(AMOUNT_CELLS.x,
                               AMOUNT_CELLS.y,
                               AMOUNT_CELLS.z,
                               false,
                               false);  // TODO replace with grid

    vec3 direction = vec3(1.0, 0.0, 0.0);
    for (int z = 0; z < AMOUNT_CELLS.z; ++z) {
        for (int y = 0; y < AMOUNT_CELLS.y; ++y) {
            Ray r;
            r.direction = direction;
            r.origin = vessel_bb.min;
            r.origin.y += ((vessel_bb.max.y - vessel_bb.min.y) / AMOUNT_CELLS.y) * (y + 0.5f);
            r.origin.z += ((vessel_bb.max.z - vessel_bb.min.z) / AMOUNT_CELLS.z) * (z + 0.5f);

            int begin_inside_idx = 0;
            bool inside = false;
            bool inside_prev = inside;

            for (int x = 0; x < AMOUNT_CELLS.x; ++x) {
                // triangle found
                if (!cells.at(x, y, z).empty()) {
                    float t_min = INFINITY;
                    float t_max = -1.0f;
                    bool min_found = false;
                    bool max_found = false;
                    size_t idx_min = 0;
                    size_t idx_max = 0;

                    for (const auto& triangle_idx : cells.at(x, y, z)) {
                        const Triangle& triangle = triangles[triangle_idx];
                        float t = triangle.intersect(r);
                        if (t < 0) continue;  // no intersection at all

                        if (t < t_min) {
                            min_found = true;
                            t_min = t;
                            idx_min = triangle_idx;
                        }

                        if (t > t_max) {
                            max_found = true;
                            t_max = t;
                            idx_max = triangle_idx;
                        }
                    }

                    if (!max_found || !min_found) continue;

                    // what happens first
                    double dot;
                    dot = glm::dot(r.direction, triangles[idx_min].normal);
                    inside = dot <= 0.0;  // true -> entering; false -> leaving

                    // what happens after it
                    dot = glm::dot(r.direction, triangles[idx_max].normal);
                    inside = dot <= 0.0;  // true -> entering; false -> leaving
                }

                if (!inside_prev && inside) {  // entered the model
                    begin_inside_idx = x;
                } else if (inside_prev && !inside) {  // left the model
                    for (int i = begin_inside_idx; i <= x; ++i) {
                        inside_cells.at(i, y, z) = true;
                    }
                }
                inside_prev = inside;
            }
        }
    }

    // erosion to reduce particles outside
    std::vector<vec3> inside_cells_eroded;
    for (int x = 1; x < AMOUNT_CELLS.x - 1; ++x) {
        for (int y = 1; y < AMOUNT_CELLS.y - 1; ++y) {
            for (int z = 1; z < AMOUNT_CELLS.z - 1; ++z) {
                bool inside_eroded =
                    inside_cells.at(x, y, z) && inside_cells.at(x + 1, y, z) &&
                    inside_cells.at(x - 1, y, z) && inside_cells.at(x, y + 1, z) &&
                    inside_cells.at(x, y - 1, z) && inside_cells.at(x, y, z + 1) &&
                    inside_cells.at(x, y, z - 1);
                if (inside_eroded) {
                    inside_cells_eroded.push_back(vec3(x, y, z));
                }
            }
        }
    }

    // add particles
    for (size_t i = 0; i < inside_cells_eroded.size(); i += inside_cells_eroded.size() / n) {
        vec3 cell = inside_cells_eroded[i];
        vec3 pos = vessel_bb.min;
        pos.x += ((vessel_bb.max.x - vessel_bb.min.x) / AMOUNT_CELLS.x) * (cell.x + 0.5f);
        pos.y += ((vessel_bb.max.y - vessel_bb.min.y) / AMOUNT_CELLS.y) * (cell.y + 0.5f);
        pos.z += ((vessel_bb.max.z - vessel_bb.min.z) / AMOUNT_CELLS.z) * (cell.z + 0.5f);
        Particle p = Particle(pos.x, pos.y, pos.z);
        p.mass = std::abs(distribution(generator));

        // add particles to particle grid
        p.particle_grid_position = particle_grid.coords(p.position);
        particle_grid.insert(p.particle_grid_position, i);
        particles.push_back(p);
    }

    // reserve memory for collisions
    collisions_particle.reserve(collisions_particle.size() + n / 2);
    collisions_vessel.reserve(n);
}

// ------------------------------------------------------------------------------------------------

// TODO improve
void Scene::update(const float dt) {
    const float max_speed = 2.0f * RADIUS_PARTICLE / dt;
    const float max_speed_sqr = max_speed * max_speed;

    // move particles
    for (auto& particle : particles) {
        particle.velocity += ((particle.acceleration + global_force) / particle.mass) * dt;

        // max speed
        if (glm::dot(particle.velocity, particle.velocity) >= max_speed_sqr) {
#ifdef GATHERING_DEBUGPRINTS
            std::cout << "too fast" << std::endl;
#endif
            particle.velocity = glm::normalize(particle.velocity) * max_speed * 0.95f;
        }

        particle.old_position = particle.position;
        particle.position += particle.velocity * dt;
        particle.bb =
            AABB(particle.position - RADIUS_PARTICLE, particle.position + RADIUS_PARTICLE);

        // clear particle grid
        // TODO use coherence?
        particle_grid.clear(particle.particle_grid_position);
    }

    for (size_t i = 0; i < particles.size(); ++i) {
        Particle& particle = particles[i];
        // update particle grid
        particle.particle_grid_position = particle_grid.coords(particle.position);
        particle_grid.insert(particle.particle_grid_position, i);
    }

    // collision with particles
    collisions_particle.clear();
    findCollisionsParticles();

    // TODO handle multiple collisions
    for (const auto& collision : collisions_particle) {
        Particle& p1 = particles[collision.first];
        Particle& p2 = particles[collision.second];

        glm::vec3 dpos = p2.position - p1.position;
        glm::vec3 dvel = p2.velocity - p1.velocity;
        double dx = glm::dot(dpos, dpos) - glm::dot(dpos + dvel, dpos + dvel);
        // particles move towards each other?
        if (dx <= 0.0) continue;

        float e = 0.5f;
        vec3 n = glm::normalize(p2.position - p1.position);
        vec3 dv = (1.f + e) * (p2.velocity - p1.velocity);
        vec3 nodge = (glm::dot(dv, n) / glm::dot(n, 2.f * n)) * n;
        p1.new_velocity += nodge;
        p2.new_velocity -= nodge;
    }

    // collision with vessel
    collisions_vessel.clear();
    findCollisionsTriangles();

    for (const size_t& particle_idx : collisions_vessel) {
        Particle& p = particles[particle_idx];
        // remove duplicate elements
        std::sort(p.close_triangles.begin(), p.close_triangles.end());
        p.close_triangles.erase(
            std::unique(p.close_triangles.begin(), p.close_triangles.end()),
            p.close_triangles.end());

        for (const size_t& triangle_idx : p.close_triangles) {
            // narrow phase
            Triangle& t = triangles[triangle_idx];
            if (!p.intersect(t)) continue;  // triangle

            // is the particle moving away from triangle?
            vec3 v = p.position - t.a;
            float distance = glm::abs(glm::dot(v, t.normal));
            float dot = glm::dot(distance * t.normal, p.velocity);
            // particle moves towards vessel?
            if (dot <= 0.0) continue;

            float e = 0.5f;
            vec3 n = t.normal;
            vec3 dv = -(1.f + e) * (p.velocity);
            vec3 nodge = (glm::dot(dv, n)) * n;
            p.new_velocity += nodge + (p.old_position - p.position) * 0.5f;
            p.position = p.position - n * glm::length(p.position - p.old_position) *
                                          1.01f;  // push away from vessel to reduce bleeding
        }
        p.close_triangles.clear();
    }

    // apply changes to particles
    for (auto& p : particles) {
        p.velocity += p.new_velocity;
        p.new_velocity = vec3(0.0);
        p.velocity /= 1.01;  // apply drag
    }
}

// ------------------------------------------------------------------------------------------------

void Scene::findCollisionsParticles() {
    for (size_t particle_idx = 0; particle_idx < particles.size(); ++particle_idx) {
        const Particle& p = particles[particle_idx];

        // find which neighbouring cells have to be checked
        glm::vec3 deviation = p.position - particle_grid.cellCenter(p.particle_grid_position);
        int neighbour_index = 0;
        if (deviation.x > 0.0f) neighbour_index |= 1;
        if (deviation.y > 0.0f) neighbour_index |= 2;
        if (deviation.z > 0.0f) neighbour_index |= 4;

        close_particles.clear();
        particle_grid.closeUniqueElements(
            p.particle_grid_position, particle_idx, neighbour_index, close_particles);

        for (const auto& particle2_idx : close_particles) {
            if (p.intersect(particles[particle2_idx])) {
                collisions_particle.push_back({particle_idx, particle2_idx});
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------

void Scene::findCollisionsTriangles() {
    bool collision_found;
    for (size_t particle_idx = 0; particle_idx < particles.size(); particle_idx++) {
        collision_found = false;
        Particle& p = particles[particle_idx];
        std::vector<vec3i> affected_cells;
        gridCoordsArea(p.bb, affected_cells);
        for (const vec3i& cell : affected_cells) {
            for (const size_t triangle_idx : cells.at(cell.x, cell.y, cell.z)) {
                const Triangle& t = triangles[triangle_idx];
                if (!p.intersect(t.bb)) continue;  // AABB
                p.close_triangles.push_back(triangle_idx);
                collision_found = true;
            }
        }
        if (collision_found) collisions_vessel.push_back(particle_idx);
    }
}

// ------------------------------------------------------------------------------------------------

void Scene::loadObject(const char* path) {
    std::cout << "Loading object ..." << std::endl;
    StopWatch<std::chrono::milliseconds> stopwatch = StopWatch<std::chrono::milliseconds>();
    std::ifstream file(path);
    if (!file) {
        printf("Failed to load shader %s\n", path);
        assert(false);
        exit(EXIT_FAILURE);
    }

    using OpenGLPrimitives::VertexData;
    std::vector<vec3> vertex_normals;

    while (!file.eof()) {
        switch (file.peek()) {
            case 'o': {
                std::string type;
                file >> type >> vessel.name;
                break;
            }

            case 'v': {
                std::string type;
                vec3 vec;
                file >> type >> vec.x >> vec.y >> vec.z;

                if (type == "v") {
                    VertexData v;
                    v.color = glm::vec4(1.f, 1.f, 1.f, 0.95f);
                    v.position = vec;
                    vessel.vertices.push_back(v);
                } else if (type == "vn") {
                    vertex_normals.push_back(glm::normalize(vec));
                } else {
                    // TODO sth went wrong
                }

                break;
            }

            case 'f': {
                std::string type, v1, v2, v3;
                file >> type >> v1 >> v2 >> v3;
                // get idx from string
                unsigned int v1_idx = std::atoi(v1.substr(0, v1.find("//")).c_str());
                unsigned int v2_idx = std::atoi(v2.substr(0, v2.find("//")).c_str());
                unsigned int v3_idx = std::atoi(v3.substr(0, v3.find("//")).c_str());
                unsigned int v1n_idx =
                    std::atoi(v1.substr(v1.find("//") + 2, v1.size()).c_str());
                unsigned int v2n_idx =
                    std::atoi(v2.substr(v2.find("//") + 2, v2.size()).c_str());
                unsigned int v3n_idx =
                    std::atoi(v3.substr(v3.find("//") + 2, v3.size()).c_str());

                // idx in obj file starts with 1
                v1_idx--;
                v2_idx--;
                v3_idx--;
                v1n_idx--;
                v2n_idx--;
                v3n_idx--;

                // add normal to vertices
                vessel.vertices[v1_idx].normal = vertex_normals[v1n_idx];
                vessel.vertices[v2_idx].normal = vertex_normals[v2n_idx];
                vessel.vertices[v3_idx].normal = vertex_normals[v3n_idx];

                // TODO vIDX != vnIDX
                // TODO IDX out of range
                // TODO v/t or v style?

                // Triangle for collision detection
                Triangle t = Triangle(vessel.vertices[v1_idx].position,
                                      vessel.vertices[v2_idx].position,
                                      vessel.vertices[v3_idx].position);
                triangles.push_back(t);

                // meta
                max_triangle_size = glm::max(t.bb.max - t.bb.min, max_triangle_size);
                min_triangle_size = glm::min(t.bb.max - t.bb.min, min_triangle_size);

                // update global AABB for vessel
                vessel_bb.max = glm::max(vessel_bb.max, t.bb.max);
                vessel_bb.min = glm::min(vessel_bb.min, t.bb.min);

                // Triangle for visualization
                vessel.elements.push_back(v1_idx);
                vessel.elements.push_back(v2_idx);
                vessel.elements.push_back(v3_idx);
                break;
            }

            default:
                std::string s;
                std::getline(file, s);
                break;
        }
    }

    // create grid
    vec3i resolution = (vessel_bb.max - vessel_bb.min) / max_triangle_size;
    grid = Grid(resolution, vessel_bb);

    // insert triangles to grid
    for (size_t i = 0; i < triangles.size(); i++) {
        const Triangle& t = triangles[i];
        std::vector<vec3i> affected_coords;
        gridCoordsArea(t.bb, affected_coords);
        for (const auto& coords : affected_coords) {
            cells.at(coords.x, coords.y, coords.z).push_back(i);
        }

        // insert to grid
        grid.insert(t.bb.min, i);
    }

#ifdef GATHERING_DEBUGPRINTS
    std::cout << "Loading time [ms]: " << stopwatch.stop() << std::endl;
#endif
}

// ------------------------------------------------------------------------------------------------

vec3i Scene::gridCoords(const glm::vec3& pos) const {
    // TODO point in AABB of vessel??
    vec3 grid_size = vessel_bb.max - vessel_bb.min;  // TODO pre compute?
    return ((pos - vessel_bb.min) * (glm::vec<3, float>)AMOUNT_CELLS) / grid_size;
}

// ------------------------------------------------------------------------------------------------

void Scene::gridCoordsArea(const AABB& aabb, std::vector<vec3i>& affected_cells) const {
    // in any cell at all?
    if (!aabb.intersect(vessel_bb)) {
        return;
    }
    affected_cells.reserve(4);

    vec3 grid_size = vessel_bb.max - vessel_bb.min;  // TODO pre compute?
    vec3i coords_min =
        ((aabb.min - vessel_bb.min) * (glm::vec<3, float>)AMOUNT_CELLS) / grid_size;
    vec3i coords_max =
        ((aabb.max - vessel_bb.min) * (glm::vec<3, float>)AMOUNT_CELLS) / grid_size;

    // objects that are equal to the vessel bb
    if (coords_max.x >= AMOUNT_CELLS.x) coords_max.x--;
    if (coords_max.y >= AMOUNT_CELLS.y) coords_max.y--;
    if (coords_max.z >= AMOUNT_CELLS.z) coords_max.z--;

    // all cells that are overlapping the bounding box
    vec3 diff = coords_max - coords_min;

    for (size_t dx = 0; dx <= diff.x; dx++) {
        for (size_t dy = 0; dy <= diff.y; dy++) {
            for (size_t dz = 0; dz <= diff.z; dz++) {
                affected_cells.emplace_back(coords_min +
                                            vec3i(dx, dy, dz));  // TODO no emplace pls!
            }
        }
    }
}

}  // namespace gathering
