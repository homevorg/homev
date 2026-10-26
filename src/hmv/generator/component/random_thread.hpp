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

#ifndef __HMV_GENERATOR_RANDOM_THREAD_HPP__
#define __HMV_GENERATOR_RANDOM_THREAD_HPP__

#include <random>
#include <set>
#include <vector>

#include "hmv/core/operation.hpp"
#include "hmv/core/random.hpp"
#include "hmv/core/random_mix.hpp"
#include "hmv/core/random_range.hpp"
#include "hmv/core/types.hpp"

namespace hmv::generator {
    class RandomThread {
     public:
        RandomThread(ThreadCount thread_count,
                     bool uniform_threads = true,
                     const RandomOperationMix& mix = {},
                     Seed seed = std::random_device{}());
        std::vector<Thread> operator()(TestSize n,
                                       const OperationMix& mix,
                                       AddressCount s);
        std::vector<Thread> operator()(Range& range, AddressCount s);
        std::vector<Thread> operator()(TestSize n, AddressCount s);
        OperationMix mix() const;

     private:
        ThreadCount thread_count;
        bool uniform_threads;
        RandomOperationMix randomized_mix;
        URNG urng;
        std::uniform_int_distribution<AddressCount> address_dist;
        std::uniform_int_distribution<ThreadCount> thread_dist;

        ThreadID select_thread(std::set<ThreadID>& threads);
        TestSize validate_n(TestSize n) const;
    };

    inline OperationMix RandomThread::mix() const {
        return randomized_mix;
    }
} // namespace hmv::generator

#endif /* __HMV_GENERATOR_RANDOM_THREAD_HPP__ */
