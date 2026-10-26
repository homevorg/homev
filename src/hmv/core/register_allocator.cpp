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

#include "register_allocator.hpp"

#include <xxhash.h>

#include "operation.hpp"
#include "test.hpp"

using hmv::Operation;
using hmv::RegisterAllocator;

void RegisterAllocator::initialize(const Test& test) {
    auto hashes = std::vector<uint64_t>{};
    hashes.reserve(test.threads.size() + 1);
    hashes.push_back(XXH3_64bits(test.addresses.data(),
                                 test.addresses.size() * sizeof(Address)));
    for (auto& thread : test.threads) {
        hashes.push_back(
            XXH3_64bits(thread.data(), thread.size() * sizeof(Operation)));
    }
    auto seed_seq = std::seed_seq{hashes.begin(), hashes.end()};
    urng.seed(seed_seq);
}

void RegisterAllocator::randomize(Test& test) {
    initialize(test);
    for (auto& thread : test.threads) {
        for (auto& op : thread) {
            op = randomize(op);
        }
    }
}
