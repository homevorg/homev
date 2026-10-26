// Copyright (C) 2019-2026 Marleson Graf

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

#ifndef __GEM5_HMV_M5OPS_H__
#define __GEM5_HMV_M5OPS_H__

#include <cstddef>
#include <cstdint>

#define GEM5_HMV_EXECUTE 0x55
#define GEM5_HMV_LOAD 0x56

enum gem5_hmv_action_t : uint32_t {
    GEM5_HMV_EXEC_BEGIN_PROGRAM = 1,
    GEM5_HMV_EXEC_BEGIN_TEST = 2,
    GEM5_HMV_EXEC_BEGIN_INSTANCE = 3,
    GEM5_HMV_EXEC_BEGIN_THREAD = 4,
    GEM5_HMV_EXEC_END_THREAD = 5,
    GEM5_HMV_EXEC_END_INSTANCE = 6,
    GEM5_HMV_EXEC_END_TEST = 7,
    GEM5_HMV_EXEC_END_PROGRAM = 8,
    GEM5_HMV_EXEC_ATOMIC_THREAD_BARRIER_WAIT = 9,
};

enum gem5_hmv_parameter_t : uint32_t {
    GEM5_HMV_PARAM_ADDRESSES = 0,
    GEM5_HMV_PARAM_TEST = 1,
    GEM5_HMV_PARAM_MAX_TEST_SIZE = 2,
};

uint64_t gem5_hmv_load(gem5_hmv_parameter_t param,
                       uint64_t value,
                       volatile void* buffer);
uint64_t gem5_hmv_execute(gem5_hmv_action_t action, uint64_t value);

#endif /* __GEM5_HMV_M5OPS_H__ */
