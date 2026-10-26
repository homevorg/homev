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

#ifndef __HMV_GENERATOR_MCVERSI_FITADDRS_COLLECTOR_HPP__
#define __HMV_GENERATOR_MCVERSI_FITADDRS_COLLECTOR_HPP__

#include <cstdint>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "hmv/core/event.hpp"
#include "hmv/core/types.hpp"

namespace hmv::generator::mcversi {
    class FitaddrsCollector {
     public:
        void add(const Event& event);
        std::unordered_set<Address> fitaddrs() const;
        double nd_t() const;
        void hard_reset();
        void soft_reset();

     private:
        struct ConflictOrder {
            // read-from order relations
            std::unordered_map<Data, std::set<Data>> rf{};
            // coherence order relations
            std::unordered_map<Data, std::set<Data>> co{};
        };
        // Total events executed in a test (max of all execution instances)
        uint64_t n{0};
        // Total events executed in an execution instance
        uint64_t n_it{0};
        // Total number of read-from and coherent order relations
        uint64_t rfco_run{0};
        // Conflict orders to each sequential address
        std::unordered_map<Address, ConflictOrder> conflicts{};
        // Last write to each sequential address
        std::unordered_map<Address, Data> last_write_id{};
        // Auxiliary program order counters to create unique ids for reads
        std::unordered_map<Address, std::vector<Data>> po_counters{};

        Data get_read_id(ThreadID tid, Address address);
    };

    inline double FitaddrsCollector::nd_t() const {
        return static_cast<double>(rfco_run) / static_cast<double>(n);
    }
} // namespace hmv::generator::mcversi

#endif /* __HMV_GENERATOR_MCVERSI_FITADDRS_COLLECTOR_HPP__ */
