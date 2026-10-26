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

#ifndef __HTS_MUTEX_HPP__
#define __HTS_MUTEX_HPP__

#include <atomic>

#include "syscall.hpp"

namespace hts {
    class Mutex {
        enum State {
            OPENED = 0,
            LOCKED = 1,
        };

     public:
        void lock() {
            while (futex.exchange(LOCKED) != OPENED) {
                wait(LOCKED);
            }
        }

        void unlock() {
            futex.store(OPENED);
            wake();
        }

     private:
        std::atomic_uint32_t futex{0};

        void wait(uint32_t expected) {
            futex_wait(reinterpret_cast<uint32_t*>(&futex), expected);
        }

        void wake() {
            futex_wake(reinterpret_cast<uint32_t*>(&futex), 1);
        }
    };
} // namespace hts

#endif /* __HTS_MUTEX_HPP__ */
