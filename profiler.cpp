/*
 *  This file is a part of KNOSSOS.
 *
 *  (C) Copyright 2007-2018
 *  Max-Planck-Gesellschaft zur Foerderung der Wissenschaften e.V.
 *
 *  KNOSSOS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 of
 *  the License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *  For further information, visit https://knossos.app
 *  or contact knossosteam@gmail.com
 */

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
