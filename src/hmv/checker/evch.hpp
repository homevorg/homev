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

#ifndef __HMV_CHECKER_EVCH_HPP__
#define __HMV_CHECKER_EVCH_HPP__

#include <list>
#include <set>
#include <unordered_map>
#include <vector>

#include "component/report.hpp"
#include "hmv/core/event.hpp"
#include "hmv/core/test.hpp"
#include "hmv/core/types.hpp"

namespace hmv::checker {
    class EVCH {
        struct Thread {
            uint64_t id;
            // Algorithm ref: B(+)
            std::list<Event> commits{};
            // Algorithm ref: B(-)
            std::list<Event> completions{};
            // Algorithm ref: B(.)
            std::list<Event> remote_commits{};
            // Algorithm ref: D^i
            std::unordered_map<Address, std::vector<Event>> coherence_orders{};
            // Algorithm ref: I^{i,q}
            std::vector<std::unordered_map<Address, unsigned>>
                coherence_orders_iterators{};
            // Algorithm ref: V_L^i
            std::unordered_map<Address, Data> commited_values{};
            // Algorithm ref: V_G^i
            std::unordered_map<Address, Data> completed_values{};
            // Algorithm ref: V_{OUT}^i
            std::unordered_map<Address, std::set<Data>> outstanding_values{};

            void record_global_value(Address address, Data value);
            void reset();
        };

     public:
        enum Error : ErrorCode {
            NONE = 0,
            AXIOM_1 = 1,
            AXIOM_2 = 2,
            AXIOM_3 = 3,
            AXIOM_5 = 5,
            AXIOM_6 = 6,
            AXIOM_8 = 8,
            AXIOM_9 = 9,
            AXIOM_11 = 11,
        };

        EVCH(ThreadCount thread_count, ISATraits traits = {});
        void begin_test(const Test& test);
        bool end_thread(ThreadID tid);
        bool end_instance();
        bool push(const ReadAt<LOCAL_COMMIT>& read);
        bool push(const WriteAt<LOCAL_COMMIT>& write);
        bool push(const FenceAt<LOCAL_COMMIT>& fence);
        bool push(const WriteAt<REMOTE_COMMIT>& write);
        bool push(const ReadAt<LOCAL_COMPLETION>& read);
        bool push(const WriteAt<LOCAL_COMPLETION>& write);
        bool push(const WriteAt<REMOTE_COMPLETION>& write);
        bool push(const ReadAt<SQUASH>& read);
        bool push(const WriteAt<SQUASH>& write);
        bool push(const WriteAt<AVAILABILITY>& write);
        const Report& report() const;

     private:
        Atomicity atomicity;
        // Algorithm ref: P
        std::unordered_map<Address, Event> last_completion{};
        std::vector<Thread> threads{};
        Report error_report{};

        bool verify_coherent_order(Thread& thread, const Event& event_u);
    };

    inline const Report& EVCH::report() const {
        return error_report;
    }
} // namespace hmv::checker

#endif /* __HMV_CHECKER_EVCH_HPP__ */
