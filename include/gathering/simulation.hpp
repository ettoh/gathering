#ifndef GATHERING_SIMULATION_H
#define GATHERING_SIMUALTION_H

#include <memory>
#include <vector>

#include "gathering/glm_include.hpp"

namespace gathering {

namespace Direction {
constexpr glm::vec3 NONE(0., 0., 0.);
constexpr glm::vec3 UP(0., 1., 0.);
constexpr glm::vec3 RIGHT(1., 0., 0.);
constexpr glm::vec3 DOWN(0., -1., 0.);
constexpr glm::vec3 LEFT(-1., 0., 0.);
constexpr glm::vec3 FRONT(0., 0., 1.);
constexpr glm::vec3 BACK(0., 0., -1.);
}  // namespace Direction

typedef std::vector<std::pair<double, glm::vec3>> ForceSchedule;

struct Resolution {
    int width;
    int height;
};

struct SimulationSettings {
    Resolution resolution = {1280, 720};
};

class Simulation {
   public:
    ~Simulation();
    Simulation(const char* file,
               const float dt,
               const SimulationSettings& settings = SimulationSettings());
    Simulation(const Simulation& a) = delete;
    Simulation& operator=(const Simulation& a) = delete;

    void addParticles(const int n, const float mass_mean, const float mass_stddev);
    void showCurrentState();
    void runTime(const int milliseconds, ForceSchedule& schedule, const bool headless);
    void runSteps(const int n, ForceSchedule& schedule, const bool headless);
    void run(ForceSchedule& schedule, const bool headless);
    void take_images(const int& slice_count);

    float dt = 0.0;

   private:
    // forward declarations
    struct SimulationImpl;
    std::unique_ptr<SimulationImpl> impl;
    void Simulation::computeFrame(ForceSchedule& schedule,
                                  const bool headless,
                                  const size_t max_frame);
    void update(const float dt);
    void findCollisionsParticles();
    void findCollisionsTriangles();
    SimulationSettings settings;
};

}  // namespace gathering

#endif
