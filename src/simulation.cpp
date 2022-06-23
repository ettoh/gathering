#include "gathering/simulation.hpp"

#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>

#include "opengl_widget.hpp"
#include "scene.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace gathering {

// --------------------------------------------------------------------------------------------

struct Simulation::SimulationImpl {
   public:
    SimulationImpl(const char* file) : scene(SceneData(file)), gl(OpenGLWidget()) {}
    SceneData scene;
    OpenGLWidget gl;
};

Simulation::~Simulation() = default;

Simulation::Simulation(const char* file, const float dt) : dt(dt) {
    impl = std::make_unique<SimulationImpl>(file);
    stbi_flip_vertically_on_write(1);
};

// --------------------------------------------------------------------------------------------

void Simulation::computeFrame(ForceSchedule& schedule,
                              const bool headless,
                              const size_t max_frame) {
    uint64_t frame_count = 0;  // for FPS; resets to 0 every second
    size_t step_count = 0;     // not reset
    std::chrono::microseconds t_sum = std::chrono::microseconds(0);
    if (!impl->gl.isPrepared()) impl->gl.prepareInstance(impl->scene);

    while (true) {
        auto t_start = std::chrono::high_resolution_clock::now();

        // 1. update scene
        update(dt);
        if (schedule.size() != 0) {
            impl->scene.global_force = schedule[0].second;
            schedule.front().first -= dt;
            if (schedule.front().first <= 0.f) {
#ifdef GATHERING_DEBUGPRINTS
                std::cout << schedule.size() << std::endl;
#endif
                schedule.erase(schedule.begin());
            }
        } else {
            impl->scene.global_force = glm::vec3(0.);
        }

        // 2. (optional) display scene
        if (!headless) {
            impl->gl.updateScene(impl->scene);
            impl->gl.renderFrame();
        }

#ifdef GATHERING_DEBUGPRINTS
        // meta
        auto t_end = std::chrono::high_resolution_clock::now();
        std::chrono::microseconds diff =
            std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start);
        t_sum += diff;
        frame_count++;
        if (t_sum.count() >= 1000000) {  // 1 sec
            t_sum /= frame_count;
            float fps = 1000000.f / t_sum.count();
            std::cout << "fps: " << fps << std::endl;
            t_sum = std::chrono::microseconds(0);
            frame_count = 0;
        }
#endif

        if (max_frame != 0) {
            if (++step_count >= max_frame) break;
        }
    }
}

// --------------------------------------------------------------------------------------------

// TODO improve
void Simulation::update(const float dt) {
    const float max_speed = 2.0f * RADIUS_PARTICLE / dt;
    const float max_speed_sqr = max_speed * max_speed;

    // move particles
    for (auto& particle : impl->scene.particles) {
        particle.velocity +=
            ((particle.acceleration + impl->scene.global_force) / particle.mass) * dt;

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
        impl->scene.particle_grid.clear(particle.particle_grid_position);
    }

    for (size_t i = 0; i < impl->scene.particles.size(); ++i) {
        Particle& particle = impl->scene.particles[i];
        // update particle grid
        particle.particle_grid_position = impl->scene.particle_grid.coords(particle.position);
        impl->scene.particle_grid.insert(particle.particle_grid_position, i);
    }

    // collision with particles
    impl->scene.collisions_particle.clear();
    findCollisionsParticles();

    // TODO handle multiple collisions
    for (const auto& collision : impl->scene.collisions_particle) {
        Particle& p1 = impl->scene.particles[collision.first];
        Particle& p2 = impl->scene.particles[collision.second];

        glm::vec3 dpos = p2.position - p1.position;
        glm::vec3 dvel = p2.velocity - p1.velocity;
        double dx = glm::dot(dpos, dpos) - glm::dot(dpos + dvel, dpos + dvel);
        // particles move towards each other?
        if (dx <= 0.0) continue;

        float e = 0.5f;
        glm::vec3 n = glm::normalize(p2.position - p1.position);
        glm::vec3 dv = (1.f + e) * (p2.velocity - p1.velocity);
        glm::vec3 nodge = (glm::dot(dv, n) / glm::dot(n, 2.f * n)) * n;
        p1.new_velocity += nodge;
        p2.new_velocity -= nodge;
    }

    // collision with vessel
    impl->scene.collisions_vessel.clear();
    findCollisionsTriangles();

    for (const size_t& particle_idx : impl->scene.collisions_vessel) {
        Particle& p = impl->scene.particles[particle_idx];
        // remove duplicate elements
        std::sort(p.close_triangles.begin(), p.close_triangles.end());
        p.close_triangles.erase(
            std::unique(p.close_triangles.begin(), p.close_triangles.end()),
            p.close_triangles.end());

        for (const size_t& triangle_idx : p.close_triangles) {
            // narrow phase
            Triangle& t = impl->scene.triangles[triangle_idx];
            if (!p.intersect(t)) continue;  // triangle

            // is the particle moving away from triangle?
            glm::vec3 v = p.position - t.a;
            float distance = glm::abs(glm::dot(v, t.normal));
            float dot = glm::dot(distance * t.normal, p.velocity);
            // particle moves towards vessel?
            if (dot <= 0.0) continue;

            float e = 0.5f;
            glm::vec3 n = t.normal;
            glm::vec3 dv = -(1.f + e) * (p.velocity);
            glm::vec3 nodge = (glm::dot(dv, n)) * n;
            p.new_velocity += nodge + (p.old_position - p.position) * 0.5f;
            p.position = p.position - n * glm::length(p.position - p.old_position) *
                                          1.01f;  // push away from vessel to reduce bleeding
        }
        p.close_triangles.clear();
    }

    // apply changes to particles
    for (auto& p : impl->scene.particles) {
        p.velocity += p.new_velocity;
        p.new_velocity = glm::vec3(0.0);
        p.velocity /= 1.01;  // apply drag
    }
}

// ------------------------------------------------------------------------------------------------

void Simulation::findCollisionsParticles() {
    for (size_t particle_idx = 0; particle_idx < impl->scene.particles.size();
         ++particle_idx) {
        const Particle& p = impl->scene.particles[particle_idx];

        // find which neighbouring cells have to be checked
        glm::vec3 deviation =
            p.position - impl->scene.particle_grid.cellCenter(p.particle_grid_position);
        int neighbour_index = 0;
        if (deviation.x > 0.0f) neighbour_index |= 1;
        if (deviation.y > 0.0f) neighbour_index |= 2;
        if (deviation.z > 0.0f) neighbour_index |= 4;

        impl->scene.close_particles.clear();
        impl->scene.particle_grid.closeUniqueElements(p.particle_grid_position,
                                                      particle_idx,
                                                      neighbour_index,
                                                      impl->scene.close_particles);

        for (const auto& particle2_idx : impl->scene.close_particles) {
            if (p.intersect(impl->scene.particles[particle2_idx])) {
                impl->scene.collisions_particle.push_back({particle_idx, particle2_idx});
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------

void Simulation::findCollisionsTriangles() {
    bool collision_found;
    for (size_t particle_idx = 0; particle_idx < impl->scene.particles.size();
         particle_idx++) {
        collision_found = false;
        Particle& p = impl->scene.particles[particle_idx];
        std::vector<vec3i> affected_cells;
        impl->scene.gridCoordsArea(p.bb, affected_cells);
        for (const vec3i& cell : affected_cells) {
            for (const size_t triangle_idx : impl->scene.cells.at(cell.x, cell.y, cell.z)) {
                const Triangle& t = impl->scene.triangles[triangle_idx];
                if (!p.intersect(t.bb)) continue;  // AABB
                p.close_triangles.push_back(triangle_idx);
                collision_found = true;
            }
        }
        if (collision_found) impl->scene.collisions_vessel.push_back(particle_idx);
    }
}

// ------------------------------------------------------------------------------------------------

void Simulation::take_images(const int& slice_count) {
    // calc camera positions for images
    glm::vec3 vessel_radius = (impl->scene.vessel_bb.max - impl->scene.vessel_bb.min) / 2.0f;
    glm::vec3 vessel_center = impl->scene.vessel_bb.min + vessel_radius;

    std::vector<glm::vec3> camera_positions;
    camera_positions.push_back(vessel_center + glm::vec3(vessel_radius.x, 0, 0));
    camera_positions.push_back(vessel_center + glm::vec3(0.1f, vessel_radius.y, 0.1f));
    camera_positions.push_back(vessel_center + glm::vec3(0, 0, vessel_radius.z));

    std::vector<glm::mat4> projections;
    projections.push_back(glm::ortho(-vessel_radius.z - 1.0f,
                                     vessel_radius.z + 1.0f,
                                     -vessel_radius.y - 1.0f,
                                     vessel_radius.y + 1.0f,
                                     -1.f,
                                     2.f * vessel_radius.z + 1.f));
    projections.push_back(glm::ortho(-vessel_radius.x - 1.0f,
                                     vessel_radius.x + 1.0f,
                                     -vessel_radius.z - 1.0f,
                                     vessel_radius.z + 1.0f,
                                     -1.f,
                                     2.f * vessel_radius.z + 1.f));
    projections.push_back(glm::ortho(-vessel_radius.x - 1.0f,
                                     vessel_radius.x + 1.0f,
                                     -vessel_radius.y - 1.0f,
                                     vessel_radius.y + 1.0f,
                                     -1.f,
                                     2.f * vessel_radius.z + 1.f));

    std::vector<glm::vec3> up_vector;
    up_vector.push_back(glm::vec3(0, 1, 0));
    up_vector.push_back(glm::vec3(0, 0, 1));
    up_vector.push_back(glm::vec3(0, 1, 0));

    impl->gl.setWindowVisibility(true);
    if (!impl->gl.isPrepared())
        impl->gl.prepareInstance(impl->scene);
    impl->gl.setImageMode(true);
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);  // viewport: x, y, width, height
    GLsizei stride = 3 * viewport[2];
    GLsizei buffer_size = stride * viewport[3];
    std::vector<char> buffer(buffer_size);

    for (int i = 0; i < 3; ++i) {
        std::string file = std::to_string(i) + ".png";
        impl->gl.setProjection(projections[i]);
        impl->gl.setView(glm::lookAt(camera_positions[i],
                                     vessel_center,
                                     up_vector[i]));  // (position, look at, up))
        impl->gl.updateScene(impl->scene);
        impl->gl.renderFrame();

        // to image
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadBuffer(GL_FRONT);
        glReadPixels(0, 0, viewport[2], viewport[3], GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
        stbi_write_png(file.c_str(), viewport[2], viewport[3], 3, &buffer[0], 3 * viewport[2]);
    }

    // slice
    float thickness = (vessel_radius.z * 2.f + 0.1f) / slice_count;
    impl->gl.setView(glm::lookAt(camera_positions[2],
                                 vessel_center,
                                 up_vector[2]));  // (position, look at, up))
    for (float i = 0; i < slice_count; ++i) {
        std::string file = "slice_" + std::to_string((int)i) + ".png";
        impl->gl.setProjection(glm::ortho(-vessel_radius.x - 1.0f,
                                          vessel_radius.x + 1.0f,
                                          -vessel_radius.z - 1.0f,
                                          vessel_radius.z + 1.0f,
                                          -0.1f + i * thickness,
                                          -0.1f + (i + 1) * thickness));
        impl->gl.updateScene(impl->scene);
        impl->gl.renderFrame();
        // to image
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadBuffer(GL_FRONT);
        glReadPixels(0, 0, viewport[2], viewport[3], GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
        stbi_write_png(file.c_str(), viewport[2], viewport[3], 3, &buffer[0], 3 * viewport[2]);
    }

    impl->gl.setImageMode(false);
}

// --------------------------------------------------------------------------------------------

void Simulation::run(ForceSchedule& schedule, const bool headless) {
    impl->gl.setWindowVisibility(!headless);
    computeFrame(schedule, headless, 0);
}

// --------------------------------------------------------------------------------------------

void Simulation::runSteps(int n, ForceSchedule& schedule, const bool headless) {
    impl->gl.setWindowVisibility(!headless);
    computeFrame(schedule, headless, n);
}

// --------------------------------------------------------------------------------------------

void Simulation::runTime(const int milliseconds,
                         ForceSchedule& schedule,
                         const bool headless) {
    impl->gl.setWindowVisibility(!headless);
    // TODO unit of dt?!
    size_t n = static_cast<size_t>(milliseconds / dt);
    computeFrame(schedule, headless, n);
}

// --------------------------------------------------------------------------------------------

void Simulation::showCurrentState() {
    impl->gl.setWindowVisibility(true);
    if (!impl->gl.isPrepared())
        impl->gl.prepareInstance(impl->scene); 

    while (true) {
        impl->gl.updateScene(impl->scene);
        impl->gl.renderFrame();
        if (impl->gl.closed()) break;
    }
}

// --------------------------------------------------------------------------------------------

void Simulation::addParticles(const int n, const float mass_mean, const float mass_stddev) {
    impl->scene.addParticles(n, mass_mean, mass_stddev);
}

}  // namespace gathering
