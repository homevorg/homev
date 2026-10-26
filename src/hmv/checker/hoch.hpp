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

#ifndef __HMV_CHECKER_HOCH_HPP__
#define __HMV_CHECKER_HOCH_HPP__

#include <list>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

#include "component/report.hpp"
#include "hmv/core/event.hpp"
#include "hmv/core/test.hpp"
#include "hmv/core/types.hpp"

namespace hmv::checker {
    class HOCH {
        struct ThreadInfo {
            // Algorithm ref: B(+)
            std::list<Event> commits{};
            // Algorithm ref: B(-)
            std::list<Event> completions{};
            // Algorithm ref: B(.)
            std::list<Event> remote_commits{};
            // Algorithm ref: V_L^i
            std::unordered_map<Address, Data> commited_values{};
            // Algorithm ref: V_{OUT}^i
            std::unordered_map<Address, std::set<Data>> outstanding_values{};

            // Algorithm ref: part of RECORD-GLOBAL-VALUE(i)
            void prune_local_values(const WriteEvent& write);
            void reset();
        };

        struct AddressInfo {
            // Algorithm ref: D_(-)
            std::vector<Event> stores{};
            // Algorithm ref: I^G
            TestSize global_index{};
            // Algorithm ref: I^{i}
            std::vector<TestSize> local_index{};

            void initialize(ThreadCount thread_count, Address address);
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

        HOCH(ThreadCount thread_count, ISATraits traits = {});
        void begin_test(const Test& test);
        bool end_thread(ThreadID tid);
        bool end_instance();
        bool end_test();
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
        std::unordered_map<Address, AddressInfo> addresses{};
        std::vector<ThreadInfo> threads{};
        Report error_report{};

        std::optional<TestSize> validate_write_order(TestSize index,
                                                     const WriteEvent& write);
        // Algorithm ref: VERIFY-CAUSAL-ORDER-ON-REMOTE-COMPLETION(i)
        bool verify_causal_order(const WriteEvent& write);
        // Algorithm ref: VERIFY-CAUSAL-ORDER-ON-READ-COMPLETION(i)
        bool verify_missing_causal_order(const ReadEvent& read);
        // Algorithm ref: VERIFY-COHERENT-ORDER-ON-REMOTE-COMPLETION(i)
        bool verify_coherent_order(const WriteEvent& write);
        // Algorithm ref: VERIFY-COHERENT-ORDER-ON-LOCAL-COMPLETION(i)
        bool verify_missing_coherent_order(const WriteEvent& write);
        // Algorithm ref: VERIFY-VALUE-ON-READ-COMMIT(i)
        bool verify_commit_value(ThreadInfo& thread, const ReadEvent& read);
        // Algorithm ref: VERIFY-VALUE-ON-READ-COMPLETION(i)
        bool verify_completion_value(ThreadInfo& thread, const ReadEvent& read);
        // Algorithm ref: part of VERIFY-ORDER-ON-LOCAL-COMMIT(i)
        bool verify_local_order(ThreadInfo& thread, const ReadEvent& read);
        // Algorithm ref: part of VERIFY-ORDER-ON-LOCAL-COMPLETION(i)
        bool verify_local_order(ThreadInfo& thread, const WriteEvent& write);
        // Algorithm ref: VERIFY-ORDER-ON-REMOTE-COMPLETION(i)
        bool verify_remote_order(const WriteEvent& write);
    };

    inline const Report& HOCH::report() const {
        return error_report;
    }
} // namespace hmv::checker

#endif /* __HMV_CHECKER_HOCH_HPP__ */
