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

#ifndef __GEM5_HMV_EVENT_MONITOR_HH__
#define __GEM5_HMV_EVENT_MONITOR_HH__

#include <cstdint>

#include "base/types.hh"
#include "cpu/o3/dyn_inst_ptr.hh"
#include "cpu/thread_context.hh"
#include "params/HMVThreadMonitor.hh"
#include "sim/sim_object.hh"

namespace gem5::ruby {
    class DataBlock;
}

namespace gem5 {
    class HMVController;

    class HMVThreadMonitor : public SimObject {
     public:
        using Params = HMVThreadMonitorParams;
        HMVThreadMonitor(const Params& p);
        void begin_thread(Addr pc);
        void end_thread();
        void
        availability(const o3::DynInstPtr& inst, uint8_t* data, unsigned size);
        void local_commit(const o3::DynInstPtr& inst);
        void local_completion_read(const o3::DynInstPtr& inst);
        void local_completion_write(const o3::DynInstPtr& inst);
        void memory_read(const o3::DynInstPtr&);
        void remote_commit(Addr paddr, const ruby::DataBlock& block);
        void remote_completion(Addr paddr, const ruby::DataBlock& block);
        void squash_read(const o3::DynInstPtr& inst);
        void squash_write(const o3::DynInstPtr& inst);
        void enter_time(const o3::DynInstPtr& inst);
        bool thread_barrier_wait(ThreadContext* tc);

     private:
        HMVController& controller;
        uint16_t tid{};
        uint64_t operation_count{0};
        bool started{false};
        Addr initial_pc{};

        uint64_t get_value(const o3::DynInstPtr& inst);
        bool is_inside_test(const o3::DynInstPtr& inst) const;
        unsigned normalize_pc(const o3::DynInstPtr& inst) const;
    };
} // namespace gem5

#endif /* __GEM5_HMV_EVENT_MONITOR_HH__ */
