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

#include "srsb.hpp"

#include <algorithm>
#include <cassert>

using hmv::checker::SRSB;

namespace hmv::checker {
    template <>
    struct ErrorInfo<SRSB::Error::NONE> {
        static constexpr auto description = "no error found";
    };

    template <>
    struct ErrorInfo<SRSB::Error::INVALID_READ> {
        static constexpr auto description =
            "read from a store not found in valid stores";
    };
} // namespace hmv::checker

SRSB::SRSB(ThreadCount thread_count, ISATraits traits) {
    assert(traits.memory_model == MCM::RELAXED);
    assert(traits.atomicity == Atomicity::OMCA);
}

void SRSB::begin_test(const Test& test) {
    valid_stores.clear();
    for (auto address : test.addresses) {
        valid_stores[address].clear();
        valid_stores[address].push_back(
            WriteAt<LOCAL_COMPLETION>{address, 0, 0, 0, 0});
    }
    error_report = {};
}

void SRSB::begin_instance() {
    for (auto& pair : valid_stores) {
        pair.second.push_back(
            WriteAt<LOCAL_COMPLETION>{pair.first, 0, 0, 0, 0});
    }
}

bool SRSB::end_instance() {
    for (auto& pair : valid_stores) {
        pair.second.clear();
    }
    invalid_stores.clear();
    return false;
}

bool SRSB::push(const ReadAt<MEMORY>& read) {
    auto addr = read.address;
    auto value = read.value;
    // Find the iterator to a valid Store with same value as the Load
    auto producer_it = std::find_if(
        std::begin(valid_stores[addr]),
        std::end(valid_stores[addr]),
        [value](const Event& store) { return value == store.value; });
    // Value Axiom
    // Check if Load value is in valid_stores for the same address
    if (producer_it == valid_stores[addr].end()) {
        error_report = make_report<Error::INVALID_READ>(read);
        return true;
    }
    // Don't update valid writes if both operations are from the same thread
    // due to oMCA store atomicity
    if (producer_it->tid != read.tid) {
        // Partial Store Serialization Axiom
        // Copy previous stores to the Store with same value to valid_stores
        invalid_stores[addr].insert(std::end(invalid_stores[addr]),
                                    std::begin(valid_stores[addr]),
                                    producer_it);
        // Erase previous stores to the Store with same value in
        // valid_stores
        valid_stores[addr].erase(std::begin(valid_stores[addr]), producer_it);
    }
    return false;
}

bool SRSB::push(const WriteAt<MEMORY>& write) {
    // Place the new store as a valid value
    valid_stores[write.address].push_back(write);
    return false;
}
