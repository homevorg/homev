// Copyright (C) 2018-2026 Marleson Graf
// Copyright (C) 2018-2019 Rafael Alevato

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

#ifndef __HMV_CHECKER_SRSB_HPP__
#define __HMV_CHECKER_SRSB_HPP__

#include <deque>
#include <unordered_map>

#include "component/report.hpp"
#include "hmv/core/event.hpp"
#include "hmv/core/test.hpp"
#include "hmv/core/types.hpp"

namespace hmv::checker {
    class SRSB {
     public:
        enum Error : ErrorCode {
            NONE = 0,
            INVALID_READ = 1
        };

        SRSB(ThreadCount thread_count, ISATraits traits = {});

        void begin_test(const Test& test);
        void begin_instance();
        bool end_instance();
        bool push(const ReadAt<MEMORY>& read);
        bool push(const WriteAt<MEMORY>& write);
        template <typename Event>
        bool check(const Event& event);
        const Report& report() const;

     private:
        Report error_report{};
        std::unordered_map<Address, std::deque<Event>> valid_stores{};
        std::unordered_map<Address, std::deque<Event>> invalid_stores{};
    };

    inline const Report& SRSB::report() const {
        return error_report;
    }
} // namespace hmv::checker

#endif // __HMV_CHECKER_SRSB_HPP__
