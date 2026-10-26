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

#ifndef __HTS_THREAD_BARRIER_HPP__
#define __HTS_THREAD_BARRIER_HPP__

#include <atomic>
#include <climits>
#include <cstdint>

#include "syscall.hpp"

namespace hts {
    class ThreadBarrier {
     public:
        constexpr ThreadBarrier(uint64_t thread_count)
            : futex{0}, counter{thread_count}, thread_count{thread_count} {}

        void wait() {
            auto expected = futex;
            if (counter.fetch_sub(1) == 1) {
                counter.store(thread_count, std::memory_order_relaxed);
                ++futex;
                futex_wake(&futex, INT_MAX);
            } else {
                while (futex == expected) {
                    futex_wait(&futex, expected);
                }
            }
        }

     private:
        uint32_t futex;
        std::atomic_uint64_t counter;
        uint64_t thread_count;
    };
} // namespace hts

#endif /* __HTS_THREAD_BARRIER_HPP__ */
