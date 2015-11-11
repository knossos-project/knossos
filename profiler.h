#ifndef PROFILER_H
#define PROFILER_H

#include <deque>
#include <chrono>
#include <cmath>

class Profiler {
public:
    void start();
    void end();
    double average_time() const;
    double average_dev() const;
private:
	std::chrono::time_point<std::chrono::steady_clock> start_time;
	std::deque<double> time_log; // in s
	std::size_t max_log_len = 60;
};

#endif//PROFILER_H
