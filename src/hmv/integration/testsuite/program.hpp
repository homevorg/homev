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

#ifndef __HTS_PROGRAM_HPP__
#define __HTS_PROGRAM_HPP__

#include "lib/thread_pool.hpp"
#include "types.hpp"

namespace hts {
    template <typename Platform>
    class Program {
     private:
        static inline WordType* base_address;

        static void reset_addresses(volatile Address* addresses) {
            auto count = Platform::get_address_count();
            for (uint64_t i = 0; i < count; ++i) {
                base_address[addresses[i] >> HTS_WORD_SHIFT] = 0;
            }
        }

        static void run_instance(ThreadFunction thread_function) {
            Platform::callback_begin_thread(thread_function);
            thread_function(base_address);
            Platform::callback_end_thread();
        }

        static void thread_main(ThreadFunction thread_function) {
            Address addresses[HTS_MAX_ADDRESS_COUNT];
            while (auto instances = Platform::callback_next_test(addresses)) {
                Platform::callback_begin_test();
                while (instances > 0) {
                    reset_addresses(addresses);
                    Platform::callback_begin_instance();
                    run_instance(thread_function);
                    Platform::callback_end_instance();
                    --instances;
                }
                Platform::callback_end_test();
            }
        }

        static int thread_other(void* arg) {
            while (auto instances = Platform::callback_next_test()) {
                while (instances > 0) {
                    run_instance(*static_cast<ThreadFunction*>(arg));
                    --instances;
                }
            }
            return 0;
        };

     public:
        static void execute() {
            ThreadFunction thread_functions[HTS_THREAD_COUNT];
            base_address = Platform::callback_begin_program(thread_functions);
            auto pool = ThreadPool<HTS_THREAD_COUNT - 1>{};
            for (auto i = 1u; i < HTS_THREAD_COUNT; ++i) {
                pool.dispatch(&thread_other, &thread_functions[i]);
            }
            thread_main(thread_functions[0]);
            pool.join_all();
            Platform::callback_end_program();
        }
    };
} // namespace hts

#endif /* __HTS_PROGRAM_HPP__ */
