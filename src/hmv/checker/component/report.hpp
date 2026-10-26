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

#ifndef __HMV_REPORT_HPP__
#define __HMV_REPORT_HPP__

#include <string>
#include <vector>

#include "hmv/core/event.hpp"

namespace hmv::checker {
    using ErrorCode = uint32_t;

    struct Report {
        ErrorCode code{};
        std::string description{};
        std::vector<Event> events{};
    };

    template <auto error>
    struct ErrorInfo {
        static constexpr auto description = "undefined result!!!";
    };

    template <auto error>
    inline Report make_report() {
        return {static_cast<ErrorCode>(error), {ErrorInfo<error>::description}};
    }

    template <auto error, typename... Events>
    inline Report make_report(const Event& event, Events&&... events) {
        return {static_cast<ErrorCode>(error),
                {ErrorInfo<error>::description},
                {event, std::forward<Events>(events)...}};
    }

    template <auto error, template <typename> typename Container>
    inline Report make_report(const Container<Event>& events) {
        return {static_cast<ErrorCode>(error),
                {ErrorInfo<error>::description},
                {events.begin(), events.end()}};
    }
} // namespace hmv::checker

#endif /* __HMV_REPORT_HPP__ */
