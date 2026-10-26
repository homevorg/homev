// Copyright (C) 2025-2026 Marleson Graf

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

#ifndef __HMV_PLATFORM_OUTPUT_HPP__
#define __HMV_PLATFORM_OUTPUT_HPP__

#include <vector>

#include "hmv/checker/checker.hpp"
#include "hmv/core/trace.hpp"
#include "hmv/core/types.hpp"
#include "hmv/coverage/metric.hpp"

namespace hmv::platform {
    struct Output {
        struct Checker {
            checker::Type type{};
            checker::ErrorCode code{};
            Time time{};
        };

        Time test_time{};
        Time warmup_time{};
        std::vector<Checker> checkers{};
        std::vector<coverage::Metric> metrics{};
        std::vector<coverage::Metric> aggregated_metrics{};
        Trace trace{};

        void serialize(std::ofstream& file) const;
        void write(const std::filesystem::path& filename) const;
        static Output deserialize(std::ifstream& file);
        static Output read(const std::filesystem::path& filename);
    };
} // namespace hmv::platform

#endif /* __HMV_PLATFORM_OUTPUT_HPP__ */
