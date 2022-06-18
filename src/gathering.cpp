#include "gathering/gathering.hpp"

#include <stdio.h>
#include <stdlib.h>

#include <chrono>
#include <iostream>

#include "opengl_widget.hpp"
#include "scene.hpp"

using glm::vec3;

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
            impl->scene.global_force = vec3(0.);
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

void Simulation::run(ForceSchedule& schedule, const bool headless) {
    impl->gl.setWindowVisibility(!headless);
    computeFrame(schedule, headless, 0);
}

void Simulation::runSteps(int n, ForceSchedule& schedule, const bool headless) {
    impl->gl.setWindowVisibility(!headless);
    computeFrame(schedule, headless, n);
}

void Simulation::runTime(const int milliseconds,
                         ForceSchedule& schedule,
                         const bool headless) {
    impl->gl.setWindowVisibility(!headless);
    // TODO unit of dt?!
    size_t n = static_cast<size_t>(milliseconds / dt);
    computeFrame(schedule, headless, n);
}

void Simulation::showCurrentState() {
    impl->gl.setWindowVisibility(true);
    impl->gl.prepareInstance(impl->scene);

    while (true) {
        impl->gl.updateScene(impl->scene);
        impl->gl.renderFrame();
        if (impl->gl.closed()) break;
    }
}

void Simulation::addParticles(const int n, const float mass_mean, const float mass_stddev) {
    impl->scene.addParticles(n, mass_mean, mass_stddev);
}

}  // namespace gathering
