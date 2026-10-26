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

#include "mrsb.hpp"
#include "hmv/core/types.hpp"

#include <algorithm>
#include <cassert>

using hmv::checker::MRSB;

namespace hmv::checker {
    template <>
    struct ErrorInfo<MRSB::Error::NONE> {
        static constexpr auto description = "no error found";
    };

    template <>
    struct ErrorInfo<MRSB::Error::UNMATCHED_EVENTS> {
        static constexpr auto description =
            "unmatched elements at the end of simulation";
    };

    template <>
    struct ErrorInfo<MRSB::Error::INVALID_PROGRAM_ORDER> {
        static constexpr auto description =
            "program order violated: received completion event different "
            "from the expected";
    };

    template <>
    struct ErrorInfo<MRSB::Error::INVALID_GLOBAL_VALUE> {
        static constexpr auto description = "Invalid global value read";
    };

    template <>
    struct ErrorInfo<MRSB::Error::INVALID_FORWARDED_VALUE> {
        static constexpr auto description =
            "value in forwarded load event is different from the value in the "
            "previous store";
    };

    template <>
    struct ErrorInfo<MRSB::Error::FORWARDED_INEXISTENT_WRITE> {
        static constexpr auto description =
            "forwarded load event received but there is no store to the same "
            "address";
    };

    template <>
    struct ErrorInfo<MRSB::Error::SWAP_MISSING_READ> {
        static constexpr auto description =
            "received write swap with no pending swap";
    };

    template <>
    struct ErrorInfo<MRSB::Error::SWAP_MISSING_WRITE> {
        static constexpr auto description =
            "received unexpected event when waiting for pending swap";
    };
} // namespace hmv::checker

MRSB::MRSB(ThreadCount thread_count, ISATraits traits)
    : memory_model{traits.memory_model}, threads{thread_count} {
    assert(traits.atomicity == Atomicity::OMCA);
}

void MRSB::begin_test(const Test& test) {
    memory.clear();
    for (auto address : test.addresses) {
        memory[address] = 0;
    }
    error_report = {};
}

bool MRSB::end_thread(ThreadID tid) {
    auto& thread = threads[tid];
    // Remove any leading fences from the commits list.
    while (!thread.commits.empty() && thread.commits.front().is_fence()) {
        thread.commits.pop_front();
    }
    if (error_report.code == Error::NONE) {
        if (!thread.commits.empty()) {
            error_report = make_report<Error::UNMATCHED_EVENTS>(thread.commits);
            return true;
        }
        if (!thread.completions.empty()) {
            error_report =
                make_report<Error::UNMATCHED_EVENTS>(thread.completions);
            return true;
        }
        if (!thread.expected_squashes.empty()) {
            error_report =
                make_report<Error::UNMATCHED_EVENTS>(thread.expected_squashes);
            return true;
        }
    }
    return false;
}

bool MRSB::end_instance() {
    for (auto& pair : memory) {
        pair.second = 0;
    }
    for (auto& thread : threads) {
        thread.reset();
    }
    return false;
}

bool MRSB::push(const WriteAt<AVAILABILITY>& write) {
    Thread& thread = threads[write.tid];
    thread.availability[write.address].insert(write.value);
    return false;
}

bool MRSB::push(const ReadAt<LOCAL_COMMIT>& read) {
    Thread& thread = threads[read.tid];
    if (thread.is_skipped(read)) {
        return check_forwarded_read(thread, read);
    }
    thread.commits.push_back(read);
    return check_matches(thread);
}

bool MRSB::push(const WriteAt<LOCAL_COMMIT>& write) {
    Thread& thread = threads[write.tid];
    thread.stores_map[write.address] = write;
    thread.commits.push_back(write);
    return check_matches(thread);
}

bool MRSB::push(const FenceAt<LOCAL_COMMIT>& fence) {
    Thread& thread = threads[fence.tid];
    thread.stores_map.clear();
    thread.commits.push_back(fence);
    return check_matches(thread);
}

bool MRSB::push(const ReadAt<LOCAL_COMPLETION>& read) {
    Thread& thread = threads[read.tid];
    if (thread.is_forwarded(read)) {
        thread.skipped_completions.push_back(read);
        return false;
    }
    thread.completions.push_back(read);
    return check_matches(thread) || check_global_value(read);
}

bool MRSB::push(const WriteAt<LOCAL_COMPLETION>& write) {
    Thread& thread = threads[write.tid];
    thread.completions.push_back(write);
    thread.clear_availability(write);
    return check_matches(thread) || check_global_value(write);
}

bool MRSB::push(const ReadAt<SQUASH>& read) {
    Thread& thread = threads[read.tid];
    if (thread.is_skipped(read)) {
        return false;
    }
    auto match = thread.expected_squashes.find(read);
    if (match != thread.expected_squashes.end()) {
        thread.expected_squashes.erase(match);
        return false;
    }
    thread.squashes.insert(read);
    return check_matches(thread);
}

bool MRSB::push(const FenceAt<SQUASH>& fence) {
    Thread& thread = threads[fence.tid];
    auto match = thread.expected_squashes.find(fence);
    if (match != thread.expected_squashes.end()) {
        thread.expected_squashes.erase(match);
        return false;
    }
    thread.squashes.insert(fence);
    return check_matches(thread);
}

bool MRSB::can_be_reordered(const Event& premature, const Event& postponed) {
    if (memory_model == MCM::TSO) {
        return postponed.is_store() && premature.is_load()
               && premature.address != postponed.address;
    }
    return !postponed.is_fence()
           && (premature.address != postponed.address
               || (premature.is_load() && postponed.is_load()));
}

bool MRSB::can_be_squashed(const Event& premature, const Event& postponed) {
    if (memory_model == MCM::TSO) {
        return premature.is_load()
               && (postponed.is_store() || postponed.is_fence());
    }
    return premature.is_load();
}

bool MRSB::check_global_value(const ReadEvent& read) {
    if (memory.count(read.address)) {
        if (memory.at(read.address) != read.value) {
            error_report = make_report<Error::INVALID_GLOBAL_VALUE>(read);
            return true;
        }
    }
    memory[read.address] = read.value;
    return false;
}

bool MRSB::check_global_value(const WriteEvent& write) {
    memory[write.address] = write.value;
    return false;
}

bool MRSB::check_forwarded_read(Thread& thread, const ReadEvent& read) {
    if (thread.stores_map.count(read.address) == 0) {
        // There is no store to this address.
        error_report = make_report<Error::FORWARDED_INEXISTENT_WRITE>(read);
        return true;
    }
    // There is a store to this address. Check if the data is compatible.
    auto& write = thread.stores_map.at(read.address);
    if (write.value != read.value) {
        error_report = make_report<Error::INVALID_FORWARDED_VALUE>(read, write);
        return true;
    }
    return false;
}

bool MRSB::check_matches(Thread& thread) {
    // Initialize with size + 1 to pass the first instance of the while.
    auto previous_size = thread.completions.size() + 1;
    // While completions is not empty and some event has been removed from
    // completions queue, keep trying to find matches.
    while (!thread.completions.empty()
           && thread.completions.size() < previous_size) {
        previous_size = thread.completions.size();
        const auto& completion = thread.completions.front();
        // Check if the event was squashed. If was, remove it from the queue
        // since there will be no equivalent commit event.
        auto match = thread.squashes.find(completion);
        if (match != thread.squashes.end()) {
            thread.squashes.erase(match);
            thread.completions.pop_front();
            continue;
        }
        // Leading fences in the commit list were respected (i.e., all
        // events before them have already been matched) and can be removed.
        while (!thread.commits.empty() && thread.commits.front().is_fence()) {
            thread.commits.pop_front();
        }
        // Go through the commits list searching for a valid match with the
        // current completion event.
        for (auto it = thread.commits.cbegin(); it != thread.commits.cend();
             ++it) {
            auto& commit = *it;
            if (commit == completion) {
                thread.commits.erase(it);
                thread.completions.pop_front();
                break;
            }
            // if the events can't be reordered...
            if (!can_be_reordered(completion, commit)) {
                // the completion event must be 'squashable'...
                if (can_be_squashed(completion, commit)) {
                    // the commit event may have caused the completion event to
                    // be squashed but the squash event didn't arrive yet.
                    // Add it to the list of events to be squashed.
                    thread.expected_squashes.insert(completion);
                    thread.completions.pop_front();
                    break;
                }
                // otherwise there is an order violation.
                error_report = make_report<Error::INVALID_PROGRAM_ORDER>(
                    completion, commit);
                return true;
            }
        }
    }
    return false;
}

void MRSB::Thread::clear_availability(const Event& event) {
    availability[event.address].erase(event.value);
}

bool MRSB::Thread::is_forwarded(const Event& event) {
    return availability[event.address].count(event.value);
}

bool MRSB::Thread::is_skipped(const Event& event) {
    auto it = std::find(
        std::begin(skipped_completions), std::end(skipped_completions), event);
    if (it != skipped_completions.end()) {
        skipped_completions.erase(it);
        return true;
    }
    return false;
}

void MRSB::Thread::reset() {
    commits.clear();
    completions = {};
    squashes.clear();
    expected_squashes.clear();
    stores_map.clear();
    availability.clear();
    skipped_completions.clear();
}
