#include "profiler.h"

#include <algorithm>
#include <numeric>

void Profiler::start() {
    start_time = std::chrono::steady_clock::now();
}

void Profiler::end() {
    std::chrono::duration<double> duration = std::chrono::steady_clock::now() - start_time;
    time_log.emplace_back(duration.count());
    if (time_log.size() > max_log_len) {
        time_log.pop_front();
    }
}

double Profiler::average_time() const {
    const double time_sum = std::accumulate(std::begin(time_log), std::end(time_log), 0.0);
    return time_sum / time_log.size();
}

double Profiler::average_dev() const {
    const double avg_time = average_time();
    const double dev_sum = std::accumulate(std::begin(time_log), std::end(time_log), 0.0, [avg_time](const double value, const double sum){
        return sum + std::abs(avg_time - value);
    });
    return dev_sum / time_log.size();
}
