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

#ifndef __GEM5_HMV_CONTROLLER_HH__
#define __GEM5_HMV_CONTROLLER_HH__

#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/types.hh"
#include "enums/ByteOrder.hh"
#include "enums/HMVFault.hh"
#include "params/HMVController.hh"
#include "sim/clocked_object.hh"

namespace hmv::platform {
    class Controller;
} // namespace hmv::platform

namespace gem5 {
    class ThreadContext;
    class System;

    class HMVController : public ClockedObject {
        class ThreadBarrier {
         public:
            ThreadBarrier(uint16_t thread_count);
            bool wait(uint16_t tid, ThreadContext* tc);

         private:
            size_t generation;
            std::vector<size_t> local_generations;
            std::unordered_set<ThreadContext*> suspended_threads;
        };

        static constexpr auto memory_padding = 1024u;

     public:
        using Params = HMVControllerParams;
        HMVController(const Params& p);
        void initState() override;
        uint64_t begin_program(uint64_t guest_word_size);
        void begin_test();
        void begin_instance();
        bool begin_thread(ThreadID tid);
        bool end_thread(ThreadID tid);
        void end_instance();
        void end_test();
        void end_program();
        template <typename Event>
        void push_event(const Event& event);
        void push_transition(const std::string& fsm,
                             const std::string& state,
                             const std::string& event,
                             const std::string& next_state,
                             Addr line_address,
                             unsigned instance_id);
        bool can_inject(HMVFault fault, Addr addr) const;
        bool thread_barrier_wait(ThreadID tid, ThreadContext* tc);
        std::optional<Addr> match_address(Addr paddr) const;
        uint64_t get_value(const void* bytes) const;
        uint64_t get_value(const void* bytes, size_t size) const;
        uint64_t load_test(ThreadContext* tc, uint64_t size, Addr vaddr);
        uint64_t load_addresses(ThreadContext* tc, uint64_t size, Addr vaddr);
        uint64_t current_time() const;
        uint64_t max_test_size() const;
        uint64_t word_size() const;

     private:
        std::unique_ptr<hmv::platform::Controller> core;
        Addr address_size;
        int guest_word_size;
        Addr physical_memory_size;
        Addr shared_memory_size;
        Addr shared_physical_region;
        Addr shared_memory_region;
        Addr code_memory_size;
        HMVFault active_fault;
        int generator_pid;
        ThreadBarrier thread_barrier;
        ByteOrder guest_byte_order{};
        std::vector<char> binary_adddresses{};
        uint64_t instance_start_time{};
        bool allow_error_injection{false};
        System* owner_system{};

        void destroy();
        bool is_first_test() const;
        void validate_address(Addr addr);
    };

    inline bool HMVController::can_inject(HMVFault fault, Addr addr) const {
        return allow_error_injection && active_fault == fault
               && addr >= shared_memory_region
               && addr < shared_memory_region + shared_memory_size;
    }

    inline uint64_t HMVController::current_time() const {
        auto current_time = curCycle() - instance_start_time;
        assert(current_time <= std::numeric_limits<uint32_t>::max());
        return current_time;
    }

    inline uint64_t HMVController::max_test_size() const {
        return code_memory_size;
    }

    inline uint64_t HMVController::word_size() const {
        return guest_word_size;
    }
} // namespace gem5

#endif /* __GEM5_HMV_CONTROLLER_HH__ */
