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

// Code adapted from https://www.marcoelver.com/files/mcversi_gem5.patch
/*
 * Copyright (c) 2015-2016, Marco Elver
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *  * Neither the name of the software nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "crossover.hpp"

#include <cstddef>
#include <unordered_set>

#include "hmv/core/types.hpp"
#include "test.hpp"

using hmv::Address;
using hmv::URNG;
using hmv::generator::mcversi::CrossoverMutate;
using hmv::generator::mcversi::ProtoOperation;
using hmv::generator::mcversi::Randomizer;
using hmv::generator::mcversi::Test;

namespace {
    double fitaddr_fraction(const Test& tg) {
        auto mem_ops = 0ul;
        auto fitaddr_ops = 0ul;
        for (auto& mem_op : tg.operations) {
            if (tg.fitaddrs.count(mem_op.address)) {
                ++fitaddr_ops;
            }
            ++mem_ops;
        }
        return static_cast<double>(fitaddr_ops) / static_cast<double>(mem_ops);
    }

    bool is_memory_access(const ProtoOperation& inst) {
        return inst.type != ProtoOperation::FENCE
               && inst.type != ProtoOperation::DELAY;
    }

    std::unordered_set<Address> merge_fitaddrs(const Test& test1,
                                               const Test& test2) {
        auto fitaddrs = test1.fitaddrs;
        for (auto& addr : test2.fitaddrs) {
            fitaddrs.insert(addr);
        }
        return fitaddrs;
    }

    void mutate(Test& test, float rate, Randomizer& random, URNG& urng) {
        auto dist =
            std::uniform_int_distribution{0ul, test.operations.size() - 1};
        auto used = std::unordered_set<size_t>{};
        auto selection_count = static_cast<size_t>(
            static_cast<float>(test.operations.size()) * rate);
        while (selection_count) {
            auto index = dist(urng);
            if (!used.count(index)) {
                test.operations[index] = random(urng);
                used.insert(index);
                --selection_count;
            }
        }
    }

    // HMV note: duplicate code from the original implementation extracted to a
    // single, generic function (no functional impact).
    bool should_select(const Test& test,
                       size_t i,
                       std::bernoulli_distribution& mem_unconditional_select,
                       std::bernoulli_distribution& nonmem_select,
                       URNG& urng) {
        auto& op = test.operations[i];
        if (is_memory_access(op)) {
            return test.fitaddrs.count(op.address)
                   || mem_unconditional_select(urng);
        }
        return nonmem_select(urng);
    }
} // namespace

CrossoverMutate::CrossoverMutate(double p_usel, double p_bfa)
    : p_usel{p_usel}, p_bfa{p_bfa} {}

void CrossoverMutate::operator()(const Test& test1,
                                 const Test& test2,
                                 float mutation_rate,
                                 std::list<Test>& population,
                                 Randomizer& random,
                                 URNG& urng) {
    // HMV note: we follow the original implementation and (re-)instantiate all
    // distributions inside this method instead of instantiating only once as
    // member fields of CrossoverMutate.
    // See CrossoverMutate::operator()(...) function (line 3859) in
    // https://www.marcoelver.com/files/mcversi_gem5.patch
    auto mem_unconditional_select = std::bernoulli_distribution{p_usel};
    auto faf1 = fitaddr_fraction(test1);
    auto faf2 = fitaddr_fraction(test2);
    auto nonmem_select1 =
        std::bernoulli_distribution{faf1 + p_usel - (faf1 * p_usel)};
    auto nonmem_select2 =
        std::bernoulli_distribution{faf2 + p_usel - (faf2 * p_usel)};
    auto prefer_test2 = std::bernoulli_distribution(0.5)(urng);
    auto new_from_fitaddrs = std::bernoulli_distribution{p_bfa};
    auto fitaddrs = merge_fitaddrs(test1, test2);
    auto child = Test{std::max(test1.generation, test2.generation) + 1,
                      test1.operations};
    auto mutations = 0ul;
    for (size_t i = 0; i < child.operations.size(); ++i) {
        auto select1 = should_select(
            test1, i, mem_unconditional_select, nonmem_select1, urng);
        auto select2 = should_select(
            test2, i, mem_unconditional_select, nonmem_select2, urng);
        // Pick gene
        if (select1 && select2) {
            if (prefer_test2) {
                child.operations[i] = test2.operations[i];
            }
        } else if (!select1 && select2) {
            child.operations[i] = test2.operations[i];
        } else if (!select1 && !select2) {
            ++mutations;
            if (new_from_fitaddrs(urng)) {
                child.operations[i] = random(urng, fitaddrs);
            } else {
                child.operations[i] = random(urng);
            }
        }
    }
    auto cur_mutation = static_cast<float>(mutations)
                        / static_cast<float>(child.operations.size());
    if (cur_mutation < mutation_rate) {
        mutate(child, mutation_rate, random, urng);
    }
    population.push_back(std::move(child));
}
