// Copyright (C) 2018-2026 Marleson Graf

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

#ifndef __HMV_CHECKER_MRSB_HPP__
#define __HMV_CHECKER_MRSB_HPP__

#include <deque>
#include <list>
#include <set>
#include <unordered_map>

#include "component/report.hpp"
#include "hmv/core/event.hpp"
#include "hmv/core/test.hpp"
#include "hmv/core/types.hpp"

namespace hmv::checker {
    class MRSB {
        struct Thread {
            std::list<Event> commits{};
            std::deque<Event> completions{};
            std::multiset<Event> squashes{};
            std::multiset<Event> expected_squashes{};
            // Used to check forwarded loads.
            std::unordered_map<Address, Event> stores_map{};
            // Used to identify unsupported forwarded loads
            std::unordered_map<Address, std::set<Data>> availability{};
            std::list<Event> skipped_completions{};

            void clear_availability(const Event& event);
            bool is_forwarded(const Event& event);
            bool is_skipped(const Event& event);
            void print();
            void reset();
        };

     public:
        enum Error : ErrorCode {
            NONE = 0,
            UNMATCHED_EVENTS = 1,
            INVALID_PROGRAM_ORDER = 2,
            INVALID_GLOBAL_VALUE = 3,
            INVALID_FORWARDED_VALUE = 4,
            FORWARDED_INEXISTENT_WRITE = 5,
            SWAP_MISSING_READ = 6,
            SWAP_MISSING_WRITE = 7
        };

        MRSB(ThreadCount thread_count, ISATraits traits = {});
        void begin_test(const Test& test);
        bool end_thread(ThreadID tid);
        bool end_instance();
        bool push(const ReadAt<LOCAL_COMMIT>& read);
        bool push(const WriteAt<LOCAL_COMMIT>& write);
        bool push(const FenceAt<LOCAL_COMMIT>& fence);
        bool push(const ReadAt<LOCAL_COMPLETION>& read);
        bool push(const WriteAt<LOCAL_COMPLETION>& write);
        bool push(const ReadAt<SQUASH>& read);
        bool push(const FenceAt<SQUASH>& fence);
        bool push(const WriteAt<AVAILABILITY>& write);
        const Report& report() const;

     private:
        MCM memory_model;
        std::vector<Thread> threads{};
        std::unordered_map<Address, Data> memory{};
        Report error_report{};

        bool can_be_reordered(const Event& premature, const Event& postponed);
        bool can_be_squashed(const Event& premature, const Event& postponed);
        bool check_forwarded_read(Thread& thread, const ReadEvent& read);
        bool check_global_value(const ReadEvent& read);
        bool check_global_value(const WriteEvent& write);
        bool check_matches(Thread& thread);
    };

    inline const Report& MRSB::report() const {
        return error_report;
    }
} // namespace hmv::checker

#endif // __HMV_CHECKER_MRSB_HPP__
