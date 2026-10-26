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

#ifndef __HTS_THREAD_POOL_HPP__
#define __HTS_THREAD_POOL_HPP__

#include <atomic>
#include <cstdint>
#include <sched.h>
#include <type_traits>

#include "arch/clone.hpp"
#include "memory.hpp"
#include "syscall.hpp"

#ifndef HTS_THREAD_STACK_SIZE
#define HTS_THREAD_STACK_SIZE 256
#endif

namespace hts {
    template <uint64_t N, uint64_t S = HTS_THREAD_STACK_SIZE>
    class ThreadPool {
        using entry_point_result_t = std::invoke_result_t<entry_point_t, void*>;
        static constexpr auto flags =
            CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD;

        enum ThreadState {
            RUNNING = 0,
            DONE = 1,
        };

     public:
        using TID = int64_t;
        using Function = int (*)(void*);

        TID dispatch(Function function, void* arg) {
            if (spawned_count < N) {
                heads[spawned_count] = spawn(function, arg);
                return spawned_count++;
            }
            return -1;
        }

        void join(TID tid) {
            if (tid < spawned_count) {
                wait(tid);
            }
        }

        void join_all() {
            for (TID i = 0; i < spawned_count; ++i) {
                wait(i);
            }
        }

     private:
        // Using clone and stack tricks from
        // https://nullprogram.com/blog/2023/03/23/
        struct alignas(16) ext_stack_head_t : public stack_head_t {
            Function user_function;
            void* arg;
            std::atomic_uint32_t state;
        };

        uint64_t spawned_count{0};
        ext_stack_head_t* heads[N]{nullptr};

        ext_stack_head_t* spawn(Function function, void* arg) const {
            auto stack_head = allocate_stack();
            stack_head_init(stack_head, &thread_entry);
            stack_head->user_function = function;
            stack_head->arg = arg;
            stack_head->state = RUNNING;
            clone(flags, stack_head);
            return stack_head;
        }

        static entry_point_result_t thread_entry(void* stack_ptr) {
            auto head = static_cast<ext_stack_head_t*>(stack_ptr);
            head->user_function(head->arg);
            head->state.store(DONE);
            futex_wake(cast_to_futex(head->state), 1);
            exit(0);
        }

        ext_stack_head_t* allocate_stack() const {
            return reinterpret_cast<ext_stack_head_t*>(malloc(S))
                   + (S / sizeof(ext_stack_head_t)) - 1;
        }

        void wait(TID tid) const {
            while (heads[tid]->state == RUNNING) {
                futex_wait(cast_to_futex(heads[tid]->state), RUNNING);
            }
        }

        static uint32_t* cast_to_futex(std::atomic_uint32_t& state) {
            return reinterpret_cast<uint32_t*>(&state);
        }
    };
} // namespace hts

#endif /* __HTS_THREAD_POOL_HPP__ */
