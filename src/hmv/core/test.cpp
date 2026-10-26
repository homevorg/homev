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

#include "test.hpp"

#include "hmv/utility/filesystem.hpp"

using hmv::Test;

void Test::serialize(std::ofstream& file) const {
    hmv::serialize(file, instances);
    hmv::serialize(file, addresses);
    hmv::serialize(file, threads.size());
    for (auto& thread : threads) {
        hmv::serialize(file, thread);
    }
}

void Test::write(const std::filesystem::path& filename) const {
    if (auto file = open_output_file(filename, std::ios::binary)) {
        serialize(file);
    }
}

Test Test::deserialize(std::ifstream& file) {
    auto test = Test{};
    if (!file.good()) {
        return test;
    }
    test.instances = hmv::deserialize<InstanceCount>(file);
    if (!file.good() || test.instances == 0) {
        return test;
    }
    test.addresses = hmv::deserialize<std::vector<Address>>(file);
    if (!file.good() || test.addresses.empty()) {
        return test;
    }
    auto thread_count = hmv::deserialize<decltype(test.threads.size())>(file);
    if (!file.good() || thread_count == 0) {
        return test;
    }
    test.threads.reserve(thread_count);
    for (auto i = size_t{0}; i < thread_count; ++i) {
        test.threads.push_back(hmv::deserialize<Thread>(file));
        if (!file.good()) {
            return test;
        }
    }
    return test;
}

Test Test::read(const std::filesystem::path& filename) {
    if (auto file = open_input_file(filename, std::ios::binary)) {
        return deserialize(file);
    }
    return {};
}
