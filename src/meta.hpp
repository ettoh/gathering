#ifndef GATHERING_META_H
#define GATHERING_META_H

#include <chrono>

namespace gathering {

template <typename TimeUnit>
class StopWatch {
   public:
    StopWatch() {
        t_end = std::chrono::high_resolution_clock::now();
        start();
    }

    void start() { t_start = std::chrono::high_resolution_clock::now(); }

    long long stop() {
        t_end = std::chrono::high_resolution_clock::now();
        auto dt = std::chrono::duration_cast<TimeUnit>(t_end - t_start);
        return dt.count();
    }

    long long round() {
        t_end = std::chrono::high_resolution_clock::now();
        auto dt = std::chrono::duration_cast<TimeUnit>(t_end - t_start);
        t_start = std::chrono::high_resolution_clock::now();
        return dt.count();
    }

   private:
    std::chrono::high_resolution_clock::time_point t_start;
    std::chrono::high_resolution_clock::time_point t_end;
};
}  // namespace gathering

#endif
