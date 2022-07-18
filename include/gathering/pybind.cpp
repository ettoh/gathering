#ifndef GATHERING_PYBIND_H
#define GATHERING_PYBIND_H

#ifdef GATHERING_PYBIND

#include <pybind11/pybind11.h>

#include <memory>
#include <string>

#include "simulation.hpp"

using namespace pybind11::literals;
namespace py = pybind11;

namespace gathering {

std::unique_ptr<Simulation> simulation_instance(nullptr);

void load(const std::string& file,
          const int cnt_particles,
          const int image_width,
          const int image_height) {
    SimulationSettings settings;
    settings.resolution = {image_width, image_height};
    simulation_instance = std::make_unique<Simulation>(file.c_str(), 0.03f, settings);
    simulation_instance->addParticles(cnt_particles, 1.0f, 0.01f);
}

void applyForce(
    const int duration, const bool headless, const float x, const float y, const float z) {
    glm::vec3 force = glm::vec3(x, y, z);
    ForceSchedule schedule = {{duration, force}};
    simulation_instance->runTime(duration, schedule, headless);
}

void setSubstepSize(const float substep) { simulation_instance->dt = substep; }

PYBIND11_MODULE(gathering, m) {
    m.def("load",
          &load,
          "Load an instance and insert k particles.",
          "file"_a,
          "cnt_particle"_a,
          "image_width"_a,
          "image_height"_a);

    m.def("applyForce", &applyForce, "duration"_a, "headless"_a, "x"_a, "y"_a, "z"_a);

    m.def("setSubstepSize", &setSubstepSize);
}

}  // namespace gathering

#endif
#endif
