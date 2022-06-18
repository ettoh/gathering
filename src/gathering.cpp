#include "gathering/gathering.hpp"

#include <stdio.h>
#include <stdlib.h>

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
    SimulationImpl(const char* file) : scene(Scene(file)), gl(OpenGLWidget()) {}
    Scene scene;
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
    impl->gl.prepareInstance(impl->scene);

    while (true) {
        auto t_start = std::chrono::high_resolution_clock::now();

        // 1. update scene
        impl->scene.update(dt);
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
