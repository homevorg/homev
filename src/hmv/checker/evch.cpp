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

#include "evch.hpp"

#include <algorithm>
#include <cassert>

using hmv::checker::EVCH;

namespace hmv::checker {
    template <>
    struct ErrorInfo<EVCH::Error::NONE> {
        static constexpr auto description = "no error found";
    };

    template <>
    struct ErrorInfo<EVCH::Error::AXIOM_1> {
        static constexpr auto description = "coherence serialization violated";
    };

    template <>
    struct ErrorInfo<EVCH::Error::AXIOM_2> {
        static constexpr auto description =
            "committed load have not completed yet";
    };

    template <>
    struct ErrorInfo<EVCH::Error::AXIOM_3> {
        static constexpr auto description =
            "completed store have not committed yet";
    };

    template <>
    struct ErrorInfo<EVCH::Error::AXIOM_5> {
        static constexpr auto description = "load completion order violated";
    };

    template <>
    struct ErrorInfo<EVCH::Error::AXIOM_6> {
        static constexpr auto description =
            "conflicting stores violate program order";
    };

    template <>
    struct ErrorInfo<EVCH::Error::AXIOM_8> {
        static constexpr auto description = "invalid value in load event";
    };

    template <>
    struct ErrorInfo<EVCH::Error::AXIOM_9> {
        static constexpr auto description =
            "program order violated across fence";
    };

    template <>
    struct ErrorInfo<EVCH::Error::AXIOM_11> {
        static constexpr auto description = "store atomicity violated";
    };
} // namespace hmv::checker

EVCH::EVCH(ThreadCount thread_count, ISATraits traits)
    : atomicity{traits.atomicity} {
    assert(traits.memory_model == MCM::RELAXED);
    threads.reserve(thread_count);
    for (uint64_t i = 0; i < thread_count; ++i) {
        threads.push_back({i});
        threads.back().coherence_orders_iterators.resize(thread_count);
    }
}

void EVCH::begin_test(const Test& test) {
    last_completion.clear();
    for (auto address : test.addresses) {
        last_completion[address] =
            WriteAt<LOCAL_COMPLETION>{address, 0, 0, 0, 0};
    }
    error_report = {};
}

bool EVCH::end_thread(ThreadID tid) {
    auto& thread = threads[tid];
    if (error_report.code == Error::NONE) {
        if (!thread.completions.empty()) {
            error_report = make_report<Error::AXIOM_2>(thread.completions);
            return true;
        }
        if (!thread.commits.empty()) {
            error_report = make_report<Error::AXIOM_3>(thread.commits);
            return true;
        }
        if (!thread.remote_commits.empty()) {
            error_report = make_report<Error::AXIOM_3>(thread.remote_commits);
            return true;
        }
    }
    return false;
}

bool EVCH::end_instance() {
    for (auto& pair : last_completion) {
        pair.second = WriteAt<LOCAL_COMPLETION>{pair.first, 0, 0, 0, 0};
    }
    for (auto& thread : threads) {
        thread.reset();
    }
    return false;
}

bool EVCH::push(const ReadAt<LOCAL_COMMIT>& read) {
    // Monitor (+)
    Thread& thread = threads[read.tid];
    // Algorithm ref: VERIFY-VALUE-ON-READ-COMMIT(i)
    if (thread.commited_values[read.address] != 0) {
        if (thread.commited_values[read.address] != read.value) {
            error_report = make_report<Error::AXIOM_8>(read);
            return true;
        }
    }
    // Algorithm ref: VERIFY-ORDER-ON-LOCAL-COMMIT(i)
    auto ej_it =
        std::find(thread.completions.begin(), thread.completions.end(), read);
    if (ej_it == thread.completions.end()) {
        error_report = make_report<Error::AXIOM_2>(read);
        return true;
    }
    thread.completions.erase(ej_it);
    return false;
}

bool EVCH::push(const WriteAt<LOCAL_COMMIT>& write) {
    // Monitor (+)
    Thread& thread = threads[write.tid];
    // Algorithm ref: RECORD-LOCAL-VALUE(i)
    thread.commited_values[write.address] = write.value;
    // Algorithm ref: VERIFY-ORDER-ON-LOCAL-COMMIT(i)
    thread.commits.push_back(write);
    return false;
}

bool EVCH::push(const FenceAt<LOCAL_COMMIT>& fence) {
    // Monitor (+)
    Thread& thread = threads[fence.tid];
    // Algorithm ref: VERIFY-FENCES(i)
    if (!thread.commits.empty()) {
        error_report = make_report<Error::AXIOM_9>(fence);
        return true;
    }
    return false;
}

bool EVCH::push(const WriteAt<REMOTE_COMMIT>& write) {
    // Monitor (.)
    Thread& thread = threads[write.tid];
    // Algorithm ref: TRACK-REMOTE-COMMIT(i)
    thread.remote_commits.push_back(write);
    return false;
}

bool EVCH::push(const ReadAt<LOCAL_COMPLETION>& read) {
    // Monitor (-)
    Thread& thread = threads[read.tid];
    // Algorithm ref: VERIFY-VALUE-ON-READ-COMPLETION(i)
    if (!thread.outstanding_values[read.address].count(read.value)) {
        if (thread.completed_values[read.address] != read.value) {
            error_report = make_report<Error::AXIOM_8>(read);
            return true;
        }
    }
    // Algorithm ref: VERIFY-ORDER-ON-LOCAL-COMPLETION(i)
    thread.completions.push_back(read);
    return false;
}

bool EVCH::push(const WriteAt<LOCAL_COMPLETION>& write) {
    // Monitor (-)
    Thread& thread = threads[write.tid];
    thread.record_global_value(write.address, write.value);
    if (verify_coherent_order(thread, write)) {
        return true;
    }
    // Algorithm ref: VERIFY-ORDER-ON-LOCAL-COMPLETION(i)
    auto ej_it = std::find(thread.commits.begin(), thread.commits.end(), write);
    if (ej_it == thread.commits.end()) {
        error_report = make_report<Error::AXIOM_3>(write);
        return true;
    }
    auto eh_it = std::find_if(
        thread.commits.begin(), ej_it, [&write](const Event& other) {
            return write.type == other.type && write.address == other.address;
        });
    if (eh_it != ej_it) {
        error_report = make_report<Error::AXIOM_6>(write);
        return true;
    }
    thread.commits.erase(ej_it);
    // Algorithm ref: TRACK-LOCAL-WRITE-COMPLETION(i)
    if (atomicity == Atomicity::OMCA) {
        last_completion[write.address] = write;
    }
    return false;
}

bool EVCH::push(const WriteAt<REMOTE_COMPLETION>& write) {
    // Monitor (X)
    Thread& thread = threads[write.tid];
    thread.record_global_value(write.address, write.value);
    if (verify_coherent_order(thread, write)) {
        return true;
    }
    // Algorithm ref: VERIFY-ORDER-ON-REMOTE-COMPLETION(i)
    auto ej_it = std::find(
        thread.remote_commits.begin(), thread.remote_commits.end(), write);
    if (ej_it == thread.remote_commits.end()) {
        error_report = make_report<Error::AXIOM_3>(write);
        return true;
    }
    thread.remote_commits.erase(ej_it);
    // Algorithm ref: VERIFY-CAUSAL-ORDER(i)
    if (atomicity == Atomicity::OMCA) {
        auto& last = last_completion[write.address];
        if (write != last) {
            error_report = make_report<Error::AXIOM_11>(last, write);
            return true;
        }
    }
    return false;
}

bool EVCH::push(const ReadAt<SQUASH>& read) {
    // Monitor (*)
    Thread& thread = threads[read.tid];
    // Algorithm ref: TRACK-SQUASH(i)
    auto em_it =
        std::find(thread.completions.begin(), thread.completions.end(), read);
    if (em_it != thread.completions.end()) {
        thread.completions.erase(em_it);
    }
    return false;
}

bool EVCH::push(const WriteAt<SQUASH>& write) {
    // Monitor (*)
    Thread& thread = threads[write.tid];
    // Algorithm ref: TRACK-SQUASH(i)
    if (thread.outstanding_values[write.address].count(write.value)) {
        thread.outstanding_values[write.address].erase(write.value);
    }
    return false;
}

bool EVCH::push(const WriteAt<AVAILABILITY>& write) {
    // Monitor (/)
    Thread& thread = threads[write.tid];
    // Algorithm ref: TRACK-AVAILABILITY
    thread.outstanding_values[write.address].insert(write.value);
    return false;
}

bool EVCH::verify_coherent_order(Thread& thread, const Event& event_u) {
    // Algorithm ref: VERIFY-COHERENT-ORDER(i)
    auto a = event_u.address;
    auto u = thread.coherence_orders[a].size();
    thread.coherence_orders[a].push_back(event_u);

    for (uint64_t q = 0; q < threads.size(); ++q) {
        if (q == thread.id || threads[q].coherence_orders[a].empty()) {
            continue;
        }
        uint64_t y = 0;
        for (auto& event_y : threads[q].coherence_orders[a]) {
            if (event_u == event_y) {
                break;
            }
            ++y;
        }
        if (y < threads[q].coherence_orders[a].size()) {
            auto is_null = !thread.coherence_orders_iterators[q].count(a);
            auto x = thread.coherence_orders_iterators[q][a];
            if (is_null || (x < y)) {
                thread.coherence_orders_iterators[q][a] = y;
                threads[q].coherence_orders_iterators[thread.id][a] = u;
            } else if (x == y) {
                // potentially duplicated cache update events
            } else {
                error_report = make_report<Error::AXIOM_1>(event_u);
                return true;
            }
        }
    }
    return false;
}

void EVCH::Thread::record_global_value(Address address, Data value) {
    // Algorithm ref: RECORD-GLOBAL-VALUE(i)
    completed_values[address] = value;
    if (outstanding_values[address].count(value)) {
        outstanding_values[address].erase(value);
    }
    if (commited_values[address] == value) {
        commited_values[address] = 0;
    }
}

void EVCH::Thread::reset() {
    commits.clear();
    completions.clear();
    remote_commits.clear();
    coherence_orders.clear();
    commited_values.clear();
    completed_values.clear();
    outstanding_values.clear();
    for (auto& iterators : coherence_orders_iterators) {
        iterators.clear();
    }
}
