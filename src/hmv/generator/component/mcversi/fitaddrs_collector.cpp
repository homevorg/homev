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

#include "fitaddrs_collector.hpp"

#include "hmv/core/types.hpp"

#include <algorithm>

using hmv::generator::mcversi::FitaddrsCollector;

void FitaddrsCollector::add(const Event& event) {
    if (event.is_load() && event.source == LOCAL_COMMIT) {
        auto read_id = get_read_id(event.tid, event.address);
        rfco_run +=
            conflicts[event.address].rf[read_id].insert(event.value).second;
        ++n_it;
    } else if (event.is_store() && event.source == LOCAL_COMPLETION) {
        rfco_run += conflicts[event.address]
                        .co[event.value]
                        .insert(last_write_id[event.address])
                        .second;
        last_write_id[event.address] = event.value;
        ++n_it;
    }
}

std::unordered_set<hmv::Address> FitaddrsCollector::fitaddrs() const {
    auto addresses = std::unordered_set<Address>{};
    auto rounded_nd_t = static_cast<size_t>(nd_t() + 0.5f);
    for (auto& [addr, orders] : conflicts) {
        auto inserted = false;
        for (auto& read : orders.rf) {
            if (read.second.size() > rounded_nd_t) {
                addresses.insert(addr);
                inserted = true;
                break;
            }
        }
        if (!inserted) {
            for (auto& write : orders.co) {
                if (write.second.size() > rounded_nd_t) {
                    addresses.insert(addr);
                    break;
                }
            }
        }
    }
    return addresses;
}

void FitaddrsCollector::hard_reset() {
    n = 0;
    n_it = 0;
    rfco_run = 0;
    conflicts.clear();
    last_write_id.clear();
    po_counters.clear();
}

void FitaddrsCollector::soft_reset() {
    for (auto& [addr, cores] : po_counters) {
        std::fill(cores.begin(), cores.end(), 0);
    }
    if (n_it > n) {
        n = n_it;
    }
    n_it = 0;
    last_write_id.clear();
}

hmv::Data FitaddrsCollector::get_read_id(ThreadID tid, Address address) {
    if (po_counters[address].size() <= tid) {
        po_counters[address].resize(tid + 1);
    }
    auto read_position = po_counters[address][tid]++;
    return read_position << 32 | tid;
}
