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

#include "gem5/m5ops.h"
#include "testsuite/lib/memory.hpp"
#include "testsuite/program.hpp"

using hts::Address;
using hts::ThreadFunction;
using hts::WordType;

namespace {
    uint64_t gem5_hmv_execute(gem5_hmv_action_t action) {
        return gem5_hmv_execute(action, 0);
    }
} // namespace

class Platform {
 public:
    static WordType* callback_begin_program(ThreadFunction* thread_functions) {
        auto base_address =
            gem5_hmv_execute(GEM5_HMV_EXEC_BEGIN_PROGRAM, sizeof(WordType));
        map_threads<0, HTS_THREAD_COUNT>(thread_functions);
        return reinterpret_cast<WordType*>(base_address);
    }

    static void callback_begin_test() {
        gem5_hmv_execute(GEM5_HMV_EXEC_BEGIN_TEST);
    }

    static void callback_begin_instance() {
        gem5_hmv_execute(GEM5_HMV_EXEC_BEGIN_INSTANCE);
    }

    static void callback_begin_thread(ThreadFunction test_function) {
        thread_barrier();
        __sync_synchronize();
        gem5_hmv_execute(GEM5_HMV_EXEC_BEGIN_THREAD,
                         reinterpret_cast<uint64_t>(test_function));
        __sync_synchronize();
    }

    static void callback_end_thread() {
        __sync_synchronize();
        gem5_hmv_execute(GEM5_HMV_EXEC_END_THREAD);
        __sync_synchronize();
        thread_barrier();
    }

    static void callback_end_instance() {
        gem5_hmv_execute(GEM5_HMV_EXEC_END_INSTANCE);
    }

    static void callback_end_test() {
        gem5_hmv_execute(GEM5_HMV_EXEC_END_TEST);
    }

    static void callback_end_program() {
        hts::free(test, test_size);
        gem5_hmv_execute(GEM5_HMV_EXEC_END_PROGRAM);
    }

    static uint64_t callback_next_test() {
        thread_barrier();
        return instance_count;
    }

    static uint64_t callback_next_test(volatile Address* addresses) {
        instance_count = gem5_hmv_load(GEM5_HMV_PARAM_TEST, test_size, test);
        if (instance_count > 0) {
            address_count = gem5_hmv_load(
                GEM5_HMV_PARAM_ADDRESSES, HTS_MAX_ADDRESS_COUNT, addresses);
        }
        return callback_next_test();
    }

    static void thread_barrier() {
        while (gem5_hmv_execute(GEM5_HMV_EXEC_ATOMIC_THREAD_BARRIER_WAIT)) {}
    }

    static uint64_t get_address_count() {
        return address_count;
    }

 private:
    static inline char* test{nullptr};
    static inline uint64_t test_size{0};
    static inline volatile uint64_t address_count{0};
    static inline volatile uint64_t instance_count{0};

    template <uint64_t I, uint64_t N>
    static void map_threads(ThreadFunction* thread_functions) {
        test_size = gem5_hmv_load(GEM5_HMV_PARAM_MAX_TEST_SIZE, 0, nullptr);
        test = hts::malloc<char>(test_size, PROT_READ | PROT_WRITE | PROT_EXEC);
        auto thread_size = test_size / N;
        for (auto i = I; i < N; ++i) {
            thread_functions[i] =
                reinterpret_cast<ThreadFunction>(&test[i * thread_size]);
        }
    }
};

int main(void) {
    hts::Program<Platform>::execute();
    hts::exit_group(0);
    return 0;
}
