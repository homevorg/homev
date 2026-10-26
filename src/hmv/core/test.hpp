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

#ifndef __HMV_CORE_TEST_HPP__
#define __HMV_CORE_TEST_HPP__

#include <filesystem>
#include <vector>

#include "operation.hpp"
#include "types.hpp"

namespace hmv {
    struct Test {
        std::vector<Address> addresses{};
        std::vector<Thread> threads{};
        InstanceCount instances{};

        void serialize(std::ofstream& file) const;
        void write(const std::filesystem::path& filename) const;
        static Test deserialize(std::ifstream& file);
        static Test read(const std::filesystem::path& filename);
    };
} // namespace hmv

#endif /* __HMV_CORE_TEST_HPP__ */
