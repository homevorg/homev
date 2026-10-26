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

#ifndef __HMV_PLATFORM_CONTROLLER_HPP__
#define __HMV_PLATFORM_CONTROLLER_HPP__

#include <set>

#include "component/tracer.hpp"
#include "hmv/checker/checker.hpp"
#include "hmv/core/test.hpp"
#include "hmv/core/time_counter.hpp"
#include "hmv/core/trace.hpp"
#include "hmv/core/types.hpp"
#include "hmv/coverage/metric.hpp"
#include "hmv/encoder/machine_code.hpp"
#include "output.hpp"

namespace hmv::platform {
    class Controller {
     public:
        Controller(MemoryTraits memory_traits,
                   ThreadCount thread_count,
                   ISATraits traits,
                   const std::vector<checker::Type>& checker_types,
                   const std::vector<coverage::Type>& coverage_types,
                   const std::vector<size_t>& coverage_totals,
                   const TraceSelection& trace_selection,
                   InstanceCount warmup_instances,
                   TestSize max_thread_size,
                   bool details = false);
        ~Controller();
        void begin_test();
        bool begin_instance();
        bool begin_thread(ThreadID tid);
        template <typename HaltCallback>
        bool end_thread(ThreadID tid, HaltCallback callback);
        template <typename HaltCallback>
        bool end_instance(HaltCallback callback);
        template <typename HaltCallback>
        void end_test(HaltCallback callback);
        InstanceCount load_test();
        ByteSequence encoded_addresses() const;
        std::vector<ByteSequence> encoded_threads();
        template <typename Event, typename HaltCallback>
        void push_event(const Event& event, HaltCallback callback);
        void push_transition(const std::string& fsm,
                             const std::string& state,
                             const std::string& event,
                             const std::string& next_state,
                             Address line_address,
                             unsigned instance_id);
        bool quiesce();
        bool match_address(Address address) const;
        size_t address_count() const;
        ByteCount max_encoded_thread_size() const;

     private:
        MemoryTraits memory_traits;
        Tracer tracer;
        InstanceCount warmup_instances;
        TestSize max_thread_size;
        bool details;
        encoder::MachineCode encoder;
        bool executing_instance{false};
        bool executing_test{false};
        std::vector<bool> executing_thread{};
        std::vector<checker::Checker> checkers{};
        std::set<Address> addresses{};
        TimeCounter test_time_counter{};
        InstanceCount warmup_count{};
        Test test{};
        Output output{};

        void reset();
        void update_output();
    };

    template <typename HaltCallback>
    inline bool Controller::end_thread(ThreadID tid, HaltCallback callback) {
        if (executing_thread[tid]) {
            for (auto it = checkers.rbegin(); it != checkers.rend(); ++it) {
                if (it->end_thread(tid)) {
                    callback(*it, tid);
                }
            }
            executing_thread[tid] = false;
            return true;
        }
        return false;
    }

    template <typename HaltCallback>
    inline bool Controller::end_instance(HaltCallback callback) {
        if (warmup_count > 0) {
            if (--warmup_count == 0) {
                output.warmup_time = test_time_counter.nanoseconds();
                test_time_counter.reset();
                test_time_counter.start();
            }
        } else if (executing_instance) {
            for (auto it = checkers.rbegin(); it != checkers.rend(); ++it) {
                if (it->end_instance()) {
                    callback(*it, 0);
                }
            }
            executing_instance = false;
            return true;
        }
        return false;
    }

    template <typename HaltCallback>
    inline void Controller::end_test(HaltCallback callback) {
        if (executing_test) {
            for (auto it = checkers.rbegin(); it != checkers.rend(); ++it) {
                if (it->end_test()) {
                    callback(*it, 0);
                }
            }
            test_time_counter.stop();
            executing_test = false;
            update_output();
            reset();
        }
    }

    template <typename Event, typename HaltCallback>
    inline void Controller::push_event(const Event& event,
                                       HaltCallback callback) {
        if (executing_thread[event.tid]) {
            for (auto& checker : checkers) {
                if (checker.push(event)) {
                    callback(checker, event.tid);
                }
            }
            tracer.push(event);
        }
    }

    inline bool Controller::match_address(Address address) const {
        return addresses.count(address);
    }

    inline size_t Controller::address_count() const {
        return addresses.size();
    }

    inline ByteCount Controller::max_encoded_thread_size() const {
        return encoder.max_thread_size(max_thread_size);
    }
} // namespace hmv::platform

#endif /* __HMV_PLATFORM_CONTROLLER_HPP__ */
