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

#ifndef __HMV_CHECKER_POCK_HPP__
#define __HMV_CHECKER_POCK_HPP__

#include <vector>

#include "component/report.hpp"
#include "hmv/core/event.hpp"
#include "hmv/core/test.hpp"
#include "hmv/core/types.hpp"

namespace hmv::checker {
    class POCK {
     public:
        enum Error : ErrorCode {
            NONE = 0,
            MISMATCH = 1,
            MISSING = 2,
            UNEXPECTED = 3,
        };

        POCK(ThreadCount thread_count, ISATraits traits = {});
        void begin_test(const Test& test);
        bool end_thread(ThreadID tid);
        bool end_instance();
        bool push(const ReadAt<LOCAL_COMMIT>& read);
        bool push(const WriteAt<LOCAL_COMMIT>& write);
        bool push(const FenceAt<LOCAL_COMMIT>& fence);
        const Report& report() const;

     private:
        std::vector<std::vector<Event>> expected_events;
        std::vector<TestSize> indices;
        Report error_report{};

        Event& expected_event(const Event& observed);
        bool test_unexpected(const Event& observed);
    };

    inline const Report& POCK::report() const {
        return error_report;
    }
} // namespace hmv::checker

#endif /* __HMV_CHECKER_POCK_HPP__ */
