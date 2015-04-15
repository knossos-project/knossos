#ifndef PROFILER_H
#define PROFILER_H

#include <deque>
#include <chrono>
#include <cmath>

class Profiler {
public:
	void start() {
		start_time = std::chrono::steady_clock::now();
	}

	void end() {
		std::chrono::duration<double> duration = std::chrono::steady_clock::now() - start_time;
		if(time_log.size() <= max_log_len) {
			time_log.emplace_back(duration.count());
		} else {
			time_log.pop_front();
			time_log.emplace_back(duration.count());
		}
	}

	double average_time() const {
		double time_sum = 0.0;
		for(const auto dt : time_log) {
			time_sum += dt;
		}
		return time_sum / time_log.size();
	}

	double average_dev() const {
		auto avg_time = average_time();
		double dev_sum = 0.0;
		for(const auto dt : time_log) {
			dev_sum += fabs(avg_time - dt);
		}
		return dev_sum / time_log.size();
	}
private:
	std::chrono::time_point<std::chrono::steady_clock> start_time;
	std::deque<double> time_log; // in s
	std::size_t max_log_len = 60;
};

#endif//PROFILER_H
