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

#ifndef __HMV_GENERATOR_MCVERSI_TEST_POOL_HPP__
#define __HMV_GENERATOR_MCVERSI_TEST_POOL_HPP__

#include <cassert>
#include <cstddef>
#include <list>
#include <random>
#include <vector>

#include "hmv/core/random.hpp"
#include "hmv/core/types.hpp"
#include "test.hpp"

namespace hmv::generator::mcversi {
    class CrossoverMutate;

    // Adaptation from the GenePool class (line 6970) in
    // https://www.marcoelver.com/files/mcversi_gem5.patch:
    class TestPool {
     public:
        TestPool(ThreadCount thread_count,
                 ByteCount memory_size,
                 ByteCount stride,
                 TestSize test_size,
                 size_t population_size,
                 float mutation_rate,
                 ByteCount chunk_size,
                 ByteCount hole_size,
                 TestSize nop_max_seq_size,
                 const OperationMix& mix,
                 Seed seed = std::random_device{}());
        std::vector<Test*> select(size_t count);
        // Code extracted from McVerSi_State::nextTest(...) function (line 8099)
        // in https://www.marcoelver.com/files/mcversi_gem5.patch
        std::vector<Test*> tournament(const std::vector<Test*>& previous,
                                      size_t tournament_size,
                                      size_t mates,
                                      CrossoverMutate& crossover_mutate);

     private:
        Randomizer random;
        size_t test_size;
        size_t population_size;
        float mutation_rate;
        URNG urng;
        std::list<Test> population{};

        std::vector<Test*> select_all();
        std::vector<Test*> select_uniform(size_t count);
        // HMV note: taking crossover_mutate as value is a mistake from the
        // original implementation, we're keeping it for the sake of fidelity,
        // although it shouldn't have any functional impact.
        // See GenePool::Step(...) function (line 7141) in
        // https://www.marcoelver.com/files/mcversi_gem5.patch
        void step(CrossoverMutate crossover_mutate,
                  const std::vector<Test*>& selection,
                  size_t mates,
                  size_t keep = 0,
                  size_t mate1_stride = 1,
                  size_t mate2_stride = 1);
    };
} // namespace hmv::generator::mcversi

#endif /* __HMV_GENERATOR_MCVERSI_TEST_POOL_HPP__ */
