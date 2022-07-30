#ifndef GATHERING_PYBIND_H
#define GATHERING_PYBIND_H

#ifdef GATHERING_PYBIND

#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <Eigen/Dense>
#include <memory>
#include <string>

#include "simulation.hpp"

using namespace pybind11::literals;
namespace py = pybind11;

namespace gathering {

typedef Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> MatrixXb;
std::unique_ptr<Simulation> simulation_instance(nullptr);
std::vector<Eigen::Map<const MatrixXb>> maps;

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

const std::vector<Eigen::Map<const MatrixXb>>& takeImages(const int slice_count) {
    ImageContainer& image_container = simulation_instance->take_images(slice_count);
    Resolution resolution = simulation_instance->getSettings().resolution;

    // assumption: image container stays valid; resolution does not change
    if (image_container.images.size() != maps.size()) {
        maps.clear();
        for (const auto& img : image_container.images) {
            maps.emplace_back(img.data(), resolution.height, resolution.width);
        }
    }
    return maps;
}

void close() { simulation_instance.release(); }

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
    m.def("takeImages", &takeImages, py::return_value_policy::reference_internal);
    m.def("close", &close);
}

}  // namespace gathering

#endif
#endif
