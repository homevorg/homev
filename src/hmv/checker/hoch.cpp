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

#include "hoch.hpp"
#include "hmv/core/types.hpp"

#include <algorithm>
#include <cassert>

using hmv::TestSize;
using hmv::checker::HOCH;

namespace hmv::checker {
    template <>
    struct ErrorInfo<HOCH::Error::NONE> {
        static constexpr auto description = "no error found";
    };

    template <>
    struct ErrorInfo<HOCH::Error::AXIOM_1> {
        static constexpr auto description = "coherence serialization violated";
    };

    template <>
    struct ErrorInfo<HOCH::Error::AXIOM_2> {
        static constexpr auto description =
            "committed load have not completed yet";
    };

    template <>
    struct ErrorInfo<HOCH::Error::AXIOM_3> {
        static constexpr auto description =
            "completed store have not committed yet";
    };

    template <>
    struct ErrorInfo<HOCH::Error::AXIOM_5> {
        static constexpr auto description = "load completion order violated";
    };

    template <>
    struct ErrorInfo<HOCH::Error::AXIOM_6> {
        static constexpr auto description =
            "conflicting stores violate program order";
    };

    template <>
    struct ErrorInfo<HOCH::Error::AXIOM_8> {
        static constexpr auto description = "invalid value in load event";
    };

    template <>
    struct ErrorInfo<HOCH::Error::AXIOM_9> {
        static constexpr auto description =
            "program order violated across fence";
    };

    template <>
    struct ErrorInfo<HOCH::Error::AXIOM_11> {
        static constexpr auto description = "store atomicity violated";
    };
} // namespace hmv::checker

HOCH::HOCH(ThreadCount thread_count, ISATraits traits)
    : atomicity{traits.atomicity}, threads{thread_count} {
    assert(traits.memory_model == MCM::RELAXED);
}

void HOCH::begin_test(const Test& test) {
    for (auto address : test.addresses) {
        addresses[address].initialize(threads.size(), address);
    }
    error_report = {};
}

bool HOCH::end_thread(ThreadID tid) {
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

bool HOCH::end_instance() {
    for (auto& [address, info] : addresses) {
        info.initialize(threads.size(), address);
    }
    for (auto& thread : threads) {
        thread.reset();
    }
    return false;
}

bool HOCH::end_test() {
    addresses.clear();
    return false;
}

bool HOCH::push(const ReadAt<LOCAL_COMMIT>& read) {
    // Monitor (+)
    auto& thread = threads[read.tid];
    return verify_commit_value(thread, read)
           || verify_local_order(thread, read);
}

bool HOCH::push(const WriteAt<LOCAL_COMMIT>& write) {
    // Monitor (+)
    auto& thread = threads[write.tid];
    // Algorithm ref: RECORD-LOCAL-VALUE(i)
    thread.commited_values[write.address] = write.value;
    // Algorithm ref: part of VERIFY-ORDER-ON-LOCAL-COMMIT(i)
    thread.commits.push_back(write);
    return false;
}

bool HOCH::push(const FenceAt<LOCAL_COMMIT>& fence) {
    // Monitor (+)
    // Algorithm ref: VERIFY-FENCES(i)
    if (!threads[fence.tid].commits.empty()) {
        error_report = make_report<Error::AXIOM_9>(fence);
        return true;
    }
    return false;
}

bool HOCH::push(const WriteAt<REMOTE_COMMIT>& write) {
    // Monitor (.)
    // Algorithm ref: TRACK-REMOTE-COMMIT(i)
    threads[write.tid].remote_commits.push_back(write);
    return false;
}

bool HOCH::push(const ReadAt<LOCAL_COMPLETION>& read) {
    // Monitor (-)
    auto& thread = threads[read.tid];
    // Algorithm ref: part of VERIFY-ORDER-ON-LOCAL-COMPLETION(i)
    thread.completions.push_back(read);
    // Common conditional extracted from the two routines below
    if (!thread.outstanding_values[read.address].count(read.value)) {
        return verify_completion_value(thread, read)
               || verify_missing_causal_order(read);
    }
    return false;
}

bool HOCH::push(const WriteAt<LOCAL_COMPLETION>& write) {
    // Monitor (-)
    auto& thread = threads[write.tid];
    // Algorithm ref: part of RECORD-GLOBAL-VALUE(i)
    thread.prune_local_values(write);
    return verify_local_order(thread, write)
           || verify_missing_coherent_order(write);
}

bool HOCH::push(const WriteAt<REMOTE_COMPLETION>& write) {
    // Monitor (X)
    return verify_remote_order(write) || verify_coherent_order(write)
           || verify_causal_order(write);
}

bool HOCH::push(const ReadAt<SQUASH>& read) {
    // Monitor (*)
    auto& thread = threads[read.tid];
    // Algorithm ref: TRACK-SQUASH(i)
    auto em_it =
        std::find(thread.completions.begin(), thread.completions.end(), read);
    if (em_it != thread.completions.end()) {
        thread.completions.erase(em_it);
    }
    return false;
}

bool HOCH::push(const WriteAt<SQUASH>& write) {
    // Monitor (*)
    // Algorithm ref: TRACK-SQUASH(i)
    threads[write.tid].outstanding_values[write.address].erase(write.value);
    return false;
}

bool HOCH::push(const WriteAt<AVAILABILITY>& write) {
    // Monitor (/)
    // Algorithm ref: TRACK-AVAILABILITY
    threads[write.tid].outstanding_values[write.address].insert(write.value);
    return false;
}

std::optional<TestSize> HOCH::validate_write_order(TestSize index,
                                                   const WriteEvent& write) {
    auto begin_it = addresses[write.address].stores.begin();
    auto end_it = addresses[write.address].stores.end();
    auto it = std::find(std::next(begin_it, index), end_it, write);
    if (it == end_it) {
        return {};
    }
    return std::distance(begin_it, it);
}

bool HOCH::verify_causal_order(const WriteEvent& write) {
    // Algorithm ref: VERIFY-CAUSAL-ORDER-ON-REMOTE-COMPLETION(i)
    if (atomicity == Atomicity::OMCA) {
        auto& info = addresses[write.address];
        auto index = info.global_index;
        if (auto updated_index = validate_write_order(index, write)) {
            info.global_index = updated_index.value();
            return false;
        }
        error_report = make_report<Error::AXIOM_11>(write, info.stores[index]);
        return true;
    }
    return false;
}

bool HOCH::verify_missing_causal_order(const ReadEvent& read) {
    // Algorithm ref: VERIFY-CAUSAL-ORDER-ON-READ-COMPLETION(i)
    if (atomicity == Atomicity::OMCA) {
        auto& info = addresses[read.address];
        if (info.local_index[read.tid] < info.global_index) {
            error_report = make_report<Error::AXIOM_11>(
                info.stores[info.global_index],
                info.stores[info.local_index[read.tid]]);
            return true;
        }
    }
    return false;
}

bool HOCH::verify_coherent_order(const WriteEvent& write) {
    // Algorithm ref: VERIFY-COHERENT-ORDER-ON-REMOTE-COMPLETION(i)
    auto& info = addresses[write.address];
    auto& cur_index = info.local_index[write.tid];
    if (auto new_index = validate_write_order(cur_index, write)) {
        cur_index = new_index.value();
        return false;
    }
    error_report = make_report<Error::AXIOM_1>(write, info.stores[cur_index]);
    return true;
}

bool HOCH::verify_missing_coherent_order(const WriteEvent& write) {
    // Algorithm ref: VERIFY-COHERENT-ORDER-ON-LOCAL-COMPLETION(i)
    auto& info = addresses[write.address];
    auto& cur_index = info.local_index[write.tid];
    auto new_index = info.stores.size();
    if (cur_index != new_index - 1) {
        error_report =
            make_report<Error::AXIOM_1>(write, info.stores[cur_index]);
        return true;
    }
    cur_index = new_index;
    info.stores.push_back(write);
    return false;
}

bool HOCH::verify_commit_value(ThreadInfo& thread, const ReadEvent& read) {
    // Algorithm ref: VERIFY-VALUE-ON-READ-COMMIT(i)
    if (thread.commited_values[read.address] != 0) {
        if (thread.commited_values[read.address] != read.value) {
            error_report = make_report<Error::AXIOM_8>(read);
            return true;
        }
    }
    return false;
}

bool HOCH::verify_completion_value(ThreadInfo& thread, const ReadEvent& read) {
    // Algorithm ref: VERIFY-VALUE-ON-READ-COMPLETION(i)
    auto& info = addresses[read.address];
    auto& last_write = info.stores[info.local_index[read.tid]];
    if (read.value != last_write.value) {
        error_report = make_report<Error::AXIOM_8>(read, last_write);
        return true;
    }
    return false;
}

bool HOCH::verify_local_order(ThreadInfo& thread, const ReadEvent& read) {
    // Algorithm ref: part of VERIFY-ORDER-ON-LOCAL-COMMIT(i)
    auto ej_it =
        std::find(thread.completions.begin(), thread.completions.end(), read);
    if (ej_it == thread.completions.end()) {
        error_report = make_report<Error::AXIOM_2>(read);
        return true;
    }
    thread.completions.erase(ej_it);
    return false;
}

bool HOCH::verify_local_order(ThreadInfo& thread, const WriteEvent& write) {
    // Algorithm ref: part of VERIFY-ORDER-ON-LOCAL-COMPLETION(i)
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
    return false;
}

bool HOCH::verify_remote_order(const WriteEvent& write) {
    // Algorithm ref: VERIFY-ORDER-ON-REMOTE-COMPLETION(i)
    auto& thread = threads[write.tid];
    auto ej_it = std::find(
        thread.remote_commits.begin(), thread.remote_commits.end(), write);
    if (ej_it == thread.remote_commits.end()) {
        error_report = make_report<Error::AXIOM_3>(write);
        return true;
    }
    thread.remote_commits.erase(ej_it);
    return false;
}

void HOCH::ThreadInfo::prune_local_values(const WriteEvent& write) {
    // Algorithm ref: part of RECORD-GLOBAL-VALUE(i)
    outstanding_values[write.address].erase(write.value);
    if (commited_values[write.address] == write.value) {
        commited_values[write.address] = 0;
    }
}

void HOCH::ThreadInfo::reset() {
    commits.clear();
    completions.clear();
    remote_commits.clear();
    commited_values.clear();
    outstanding_values.clear();
}

void HOCH::AddressInfo::initialize(ThreadCount thread_count, Address address) {
    stores.clear();
    stores.push_back(WriteAt<LOCAL_COMPLETION>{address, 0, 0, 0, 0});
    global_index = 0;
    local_index.clear();
    local_index.resize(thread_count, 0);
}
