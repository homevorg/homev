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

#include "pock.hpp"

using hmv::Event;
using hmv::Test;
using hmv::checker::POCK;

namespace hmv::checker {
    template <>
    struct ErrorInfo<POCK::Error::NONE> {
        static constexpr auto description = "no error found";
    };

    template <>
    struct ErrorInfo<POCK::Error::MISMATCH> {
        static constexpr auto description =
            "mismatch between expected operation and observed commit event";
    };

    template <>
    struct ErrorInfo<POCK::Error::MISSING> {
        static constexpr auto description =
            "missing one or more expected operations";
    };

    template <>
    struct ErrorInfo<POCK::Error::UNEXPECTED> {
        static constexpr auto description =
            "found one or more unexpected operations";
    };
} // namespace hmv::checker

POCK::POCK(ThreadCount thread_count, ISATraits traits)
    : expected_events(thread_count), indices(thread_count, 0) {}

void POCK::begin_test(const Test& test) {
    for (auto i = ThreadID{0}; i < expected_events.size(); ++i) {
        expected_events[i].clear();
        expected_events[i].reserve(test.threads[i].size());
        for (auto& op : test.threads[i]) {
            if (op.type < Operation::NOP) {
                expected_events[i].push_back({test.addresses[op.address],
                                              op.value,
                                              op.type,
                                              Monitor::NONE,
                                              i});
            }
        }
    }
    error_report = {};
}

bool POCK::end_thread(ThreadID tid) {
    if (error_report.code == Error::NONE) {
        if (indices[tid] != expected_events[tid].size()) {
            if (indices[tid] < expected_events[tid].size()) {
                error_report = make_report<Error::MISSING>();
            } else {
                error_report = make_report<Error::UNEXPECTED>();
            }
            return true;
        }
    }
    return false;
}

bool POCK::end_instance() {
    indices.assign(indices.size(), 0);
    return false;
}

bool POCK::push(const ReadAt<LOCAL_COMMIT>& read) {
    if (test_unexpected(read)) {
        return true;
    }
    auto& expected = expected_event(read);
    if (read.type != expected.type || read.address != expected.address) {
        error_report = make_report<Error::MISMATCH>(read, expected);
        return true;
    }
    return false;
}

bool POCK::push(const WriteAt<LOCAL_COMMIT>& write) {
    if (test_unexpected(write)) {
        return true;
    }
    auto& expected = expected_event(write);
    if (write != expected) {
        error_report = make_report<Error::MISMATCH>(write, expected);
        return true;
    }
    return false;
}

bool POCK::push(const FenceAt<LOCAL_COMMIT>& fence) {
    if (test_unexpected(fence)) {
        return true;
    }
    auto& expected = expected_event(fence);
    if (fence.type != expected.type) {
        error_report = make_report<Error::MISMATCH>(fence, expected);
        return true;
    }
    return false;
}

Event& POCK::expected_event(const Event& observed) {
    return expected_events[observed.tid][indices[observed.tid]++];
}

bool POCK::test_unexpected(const Event& observed) {
    if (indices[observed.tid] >= expected_events[observed.tid].size()) {
        error_report = make_report<Error::UNEXPECTED>();
        return true;
    }
    return false;
}
