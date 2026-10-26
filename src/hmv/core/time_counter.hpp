// Copyright (C) 2018-2026 Marleson Graf

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#ifndef __HMV_CORE_TIME_COUNTER_HPP__
#define __HMV_CORE_TIME_COUNTER_HPP__

#include <chrono>

#include "types.hpp"

namespace hmv {
    class TimeCounter {
        using Duration = std::chrono::duration<double>;

     public:
        void reset();
        void start();
        void stop();
        Time milliseconds() const;
        Time nanoseconds() const;
        double seconds() const;

     private:
        bool running{false};
        std::chrono::steady_clock::time_point start_time{};
        std::chrono::duration<double> elapsed_time{};

        Duration duration() const;
    };

    inline void TimeCounter::reset() {
        running = false;
        start_time = {};
        elapsed_time = {};
    }

    inline void TimeCounter::start() {
        if (!running) {
            start_time = std::chrono::steady_clock::now();
            running = true;
        }
    }

    inline void TimeCounter::stop() {
        if (running) {
            auto end_time = std::chrono::steady_clock::now();
            elapsed_time += end_time - start_time;
            running = false;
        }
    }

    inline Time TimeCounter::milliseconds() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration())
            .count();
    }

    inline Time TimeCounter::nanoseconds() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(duration())
            .count();
    }

    inline double TimeCounter::seconds() const {
        using namespace std::literals;
        return duration() / 1.0s;
    }

    inline TimeCounter::Duration TimeCounter::duration() const {
        if (!running) {
            return elapsed_time;
        }
        return elapsed_time + std::chrono::steady_clock::now() - start_time;
    }
} // namespace hmv

#endif /* __HMV_CORE_TIME_COUNTER_HPP__ */
