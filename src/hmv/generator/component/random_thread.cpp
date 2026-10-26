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

#include "random_thread.hpp"

#include <cassert>

using hmv::TestSize;
using hmv::Thread;
using hmv::ThreadID;
using hmv::generator::RandomThread;

RandomThread::RandomThread(ThreadCount thread_count,
                           bool uniform_threads,
                           const RandomOperationMix& mix,
                           Seed seed)
    : thread_count{thread_count}, uniform_threads{uniform_threads},
      randomized_mix{mix}, urng{seed} {}

std::vector<Thread> RandomThread::operator()(TestSize n,
                                             const OperationMix& mix,
                                             AddressCount s) {
    randomized_mix = mix;
    return operator()(n, s);
}

std::vector<Thread> RandomThread::operator()(Range& range, AddressCount s) {
    return operator()(range(urng), s);
}

std::vector<Thread> RandomThread::operator()(TestSize n, AddressCount s) {
    randomized_mix.randomize(urng);
    n = validate_n(n);
    set_param<AddressCount>(address_dist, 0, s - 1);
    auto threads = std::vector<Thread>{thread_count};
    auto value = Data{1};
    auto available_threads = std::set<ThreadID>{};
    for (auto tid = ThreadID{0}; tid < thread_count; ++tid) {
        available_threads.insert(tid);
    }
    for (auto i = 0u; i < n; ++i) {
        auto tid = select_thread(available_threads);
        switch (randomized_mix(urng)) {
        case Operation::LOAD:
            threads[tid].push_back(Load{address_dist(urng)});
            break;
        case Operation::STORE:
            threads[tid].push_back(Store{address_dist(urng), value++});
            break;
        case Operation::FENCE:
            threads[tid].push_back(Fence{});
            break;
        case Operation::NOP:
            threads[tid].push_back(Nop{});
            break;
        default:
            assert(false);
            break;
        }
        if (uniform_threads && threads[tid].size() == n / thread_count) {
            available_threads.erase(tid);
        }
    }
    return threads;
}

ThreadID RandomThread::select_thread(std::set<ThreadID>& threads) {
    set_param<ThreadCount>(thread_dist, 0, threads.size() - 1);
    return *std::next(threads.begin(), thread_dist(urng));
}

TestSize RandomThread::validate_n(TestSize n) const {
    n = std::max(n, static_cast<TestSize>(thread_count));
    if (uniform_threads) {
        return n + n % thread_count;
    }
    return n;
}
