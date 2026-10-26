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

#include "output.hpp"

#include "hmv/utility/filesystem.hpp"

using hmv::platform::Output;

void Output::serialize(std::ofstream& file) const {
    hmv::serialize(file, test_time);
    hmv::serialize(file, warmup_time);
    hmv::serialize(file, checkers);
    hmv::serialize(file, metrics.size());
    for (auto i = size_t{0}; i < metrics.size(); ++i) {
        metrics[i].serialize(file);
        aggregated_metrics[i].serialize(file);
    }
    trace.serialize(file);
}

void Output::write(const std::filesystem::path& filename) const {
    if (auto file =
            open_output_file(filename, std::ios::binary | std::ios::trunc)) {
        serialize(file);
    }
}

Output Output::deserialize(std::ifstream& file) {
    auto output = Output{};
    if (!file.good()) {
        return output;
    }
    output.test_time = hmv::deserialize<decltype(output.test_time)>(file);
    output.warmup_time = hmv::deserialize<decltype(output.warmup_time)>(file);
    output.checkers = hmv::deserialize<std::vector<Checker>>(file);
    auto metrics_count =
        hmv::deserialize<decltype(output.metrics.size())>(file);
    output.metrics.reserve(metrics_count);
    output.aggregated_metrics.reserve(metrics_count);
    for (auto i = size_t{0}; i < metrics_count; ++i) {
        output.metrics.push_back(coverage::Metric::deserialize(file));
        output.aggregated_metrics.push_back(
            coverage::Metric::deserialize(file));
    }
    output.trace = Trace::deserialize(file);
    return output;
}

Output Output::read(const std::filesystem::path& filename) {
    if (auto file = open_input_file(filename, std::ios::binary)) {
        return deserialize(file);
    }
    return {};
}
