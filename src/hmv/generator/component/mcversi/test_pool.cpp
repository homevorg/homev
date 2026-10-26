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
 * Copyright (c) 2014-2016, Marco Elver
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

#include "test_pool.hpp"

#include <unordered_set>

#include "crossover.hpp"

using hmv::generator::mcversi::Test;
using hmv::generator::mcversi::TestPool;

TestPool::TestPool(ThreadCount thread_count,
                   ByteCount memory_size,
                   ByteCount stride,
                   TestSize test_size,
                   size_t population_size,
                   float mutation_rate,
                   ByteCount chunk_size,
                   ByteCount hole_size,
                   TestSize nop_max_seq_size,
                   const OperationMix& mix,
                   Seed seed)
    : random{thread_count,
             memory_size,
             stride,
             chunk_size,
             hole_size,
             nop_max_seq_size,
             mix},
      test_size{test_size}, population_size{population_size},
      mutation_rate{mutation_rate}, urng{seed} {
    for (auto i = 0u; i < population_size; ++i) {
        population.emplace_back(Test::make_random(test_size, random, urng));
    }
}

std::vector<Test*> TestPool::select(size_t count) {
    return population_size == 1 ? select_all() : select_uniform(count);
}

std::vector<Test*> TestPool::tournament(const std::vector<Test*>& previous,
                                        size_t tournament_size,
                                        size_t mates,
                                        CrossoverMutate& crossover_mutate) {
    if (population_size == 1) {
        assert(population.size() == 1);
        population.clear();
        population.push_back(Test::make_random(test_size, random, urng));
        assert(previous.size() == 1);
        return select_all();
    }
    auto winners = std::vector<Test*>{};
    for (size_t i = 0; i < previous.size(); i += tournament_size) {
        auto winner = previous[i];
        for (size_t j = 1; j < tournament_size; ++j) {
            // HMV note: this error is from the original implementation!
            // previous[3] always looses, and crossover of a test with itself
            // might happen. It should be previous[i + j] instead of
            // previous[j].
            // See McVerSi_State::nextTest(...) function (line 8099) in
            // https://www.marcoelver.com/files/mcversi_gem5.patch
            if (previous[j]->fitness > winner->fitness) {
                winner = previous[j];
            }
        }
        winners.push_back(winner);
    }
    assert(winners.size() == mates);
    winners.push_back(&population.front());
    step(crossover_mutate, winners, mates, mates);
    assert(population.size() == population_size);
    return select_uniform(previous.size());
}

std::vector<Test*> TestPool::select_all() {
    auto result = std::vector<Test*>{};
    result.reserve(population.size());
    for (auto& g : population) {
        result.push_back(&g);
    }
    return result;
}

std::vector<Test*> TestPool::select_uniform(size_t count) {
    auto dist = std::uniform_int_distribution{0ul, population.size() - 1};
    auto used = std::unordered_set<size_t>{};
    auto result = std::vector<Test*>{};
    result.reserve(count);
    while (result.size() < count) {
        auto idx = dist(urng);
        if (used.insert(idx).second) {
            auto it = std::next(population.begin(), idx);
            result.push_back(&(*it));
        }
    }
    return result;
}

void TestPool::step(CrossoverMutate crossover_mutate,
                    const std::vector<Test*>& winners,
                    size_t mates,
                    size_t keep,
                    size_t mate1_stride,
                    size_t mate2_stride) {
    assert(winners.size() >= mates);
    assert(winners.size() >= keep);
    auto target_size = population_size + winners.size() - keep;
    auto target_reached = false;
    for (size_t i = 0; i < mates; i += mate1_stride) {
        for (size_t j = i + 1; j < mates; j += mate2_stride) {
            crossover_mutate(*winners[i],
                             *winners[j],
                             mutation_rate,
                             population,
                             random,
                             urng);
            if (population.size() >= target_size) {
                target_reached = true;
                break;
            }
        }
        if (target_reached) {
            break;
        }
    }
    for (auto it = std::next(winners.begin(), keep); it != winners.end();
         ++it) {
        for (auto p_it = population.begin(); p_it != population.end(); ++p_it) {
            if (*it == &(*p_it)) {
                p_it = population.erase(p_it);
                break;
            }
        }
    }
    assert(population.size() >= population_size);
}
