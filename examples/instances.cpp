#include <gathering/simulation.hpp>

int main() {
    gathering::Simulation simulation = gathering::Simulation("cut1_2.obj", 0.03f);
    gathering::ForceSchedule force_schedule;  // list of <duration, force>
    float force = 0.1f;
    force_schedule.push_back({3, gathering::Direction::NONE});
    force_schedule.push_back({5, gathering::Direction::UP * force});
    force_schedule.push_back({5, gathering::Direction::UP * force});
    force_schedule.push_back({5, gathering::Direction::UP * force});
    force_schedule.push_back({5, gathering::Direction::UP * force});
    force_schedule.push_back({5, gathering::Direction::UP * force});
    force_schedule.push_back({2, gathering::Direction::RIGHT * force});
    force_schedule.push_back({1, gathering::Direction::LEFT * force});
    force_schedule.push_back({250, gathering::Direction::UP * force});
    force_schedule.push_back({100, gathering::Direction::DOWN * force});

    simulation.addParticles(5000, 1.0f, 0.01f);
    // simulation.runTime(20, force_schedule, true);
    // simulation.showCurrentState();
    // simulation.run(force_schedule, true);
    simulation.runTime(30, force_schedule, false);
    simulation.take_images(6);
    // simulation.showCurrentState();
    simulation.run(force_schedule, false);

    return 0;
}