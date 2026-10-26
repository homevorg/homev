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

#include "pseudo_inst.hh"

#include "cpu/base.hh"
#include "cpu/thread_context.hh"
#include "sim/system.hh"

#include "hmv_controller.hh"
#include "hmv_thread_monitor.hh"
#include "m5ops.h"

uint64_t gem5::pseudo_inst::hmvexecute(ThreadContext* tc,
                                       uint64_t action,
                                       uint64_t value) {
    auto hmvController = tc->getSystemPtr()->hmvController;
    auto hmvThreadMonitor = tc->getCpuPtr()->hmvThreadMonitor;
    if (hmvController == nullptr || hmvThreadMonitor == nullptr) {
        return 0;
    }
    switch (static_cast<gem5_hmv_action_t>(action)) {
    case GEM5_HMV_EXEC_BEGIN_PROGRAM:
        return hmvController->begin_program(value);
    case GEM5_HMV_EXEC_BEGIN_TEST:
        hmvController->begin_test();
        break;
    case GEM5_HMV_EXEC_BEGIN_INSTANCE:
        hmvController->begin_instance();
        break;
    case GEM5_HMV_EXEC_BEGIN_THREAD:
        hmvThreadMonitor->begin_thread(value);
        break;
    case GEM5_HMV_EXEC_END_THREAD:
        hmvThreadMonitor->end_thread();
        break;
    case GEM5_HMV_EXEC_END_INSTANCE:
        hmvController->end_instance();
        break;
    case GEM5_HMV_EXEC_END_TEST:
        hmvController->end_test();
        break;
    case GEM5_HMV_EXEC_END_PROGRAM:
        hmvController->end_program();
        break;
    case GEM5_HMV_EXEC_ATOMIC_THREAD_BARRIER_WAIT:
        return hmvThreadMonitor->thread_barrier_wait(tc);
    }
    return 0;
}

uint64_t gem5::pseudo_inst::hmvload(ThreadContext* tc,
                                    uint64_t param,
                                    uint64_t value,
                                    Addr buffer) {
    auto hmvController = tc->getSystemPtr()->hmvController;
    if (hmvController == nullptr) {
        return 0;
    }
    switch (static_cast<gem5_hmv_parameter_t>(param)) {
    case GEM5_HMV_PARAM_ADDRESSES:
        return hmvController->load_addresses(tc, value, buffer);
    case GEM5_HMV_PARAM_TEST:
        return hmvController->load_test(tc, value, buffer);
    case GEM5_HMV_PARAM_MAX_TEST_SIZE:
        return hmvController->max_test_size();
    default:
        return 0;
    }
}
