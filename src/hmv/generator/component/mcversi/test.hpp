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

#ifndef __HMV_GENERATOR_MCVERSI_TEST_HPP__
#define __HMV_GENERATOR_MCVERSI_TEST_HPP__

#include <random>
#include <unordered_set>
#include <vector>

#include "hmv/core/random.hpp"
#include "hmv/core/random_mix.hpp"
#include "hmv/core/types.hpp"

namespace hmv::generator::mcversi {
    struct ProtoOperation {
        enum Type : uint8_t {
            READ = 0,
            READ_ADDR_DEP = 1,
            WRITE = 2,
            FENCE = 3,
            DELAY = 4,
        };

        Type type{};
        Address address{};
        ThreadID tid{};
    };

    using OperationMix = RatioMix<ProtoOperation::Type,
                                  ProtoOperation::READ,
                                  ProtoOperation::READ_ADDR_DEP,
                                  ProtoOperation::WRITE,
                                  ProtoOperation::FENCE,
                                  ProtoOperation::DELAY>;
    using SelectionOperationMix =
        SelectionRatioMix<ProtoOperation::Type,
                          ProtoOperation::READ,
                          ProtoOperation::READ_ADDR_DEP,
                          ProtoOperation::WRITE,
                          ProtoOperation::FENCE,
                          ProtoOperation::DELAY>;

    class Randomizer {
     public:
        Randomizer(ThreadCount thread_count,
                   ByteCount usable_memory_size,
                   ByteCount stride,
                   ByteCount chunk_size,
                   ByteCount hole_size,
                   TestSize nop_max_seq_size,
                   const OperationMix& mix);
        ProtoOperation operator()(URNG& urng);
        ProtoOperation operator()(URNG& urng,
                                  const std::unordered_set<Address>& addresses);

     private:
        Address stride_mask;
        ByteCount hole_size;
        SelectionOperationMix mix;
        std::uniform_int_distribution<Address> addr_dist;
        std::uniform_int_distribution<Address> chunk_dist;
        std::uniform_int_distribution<ThreadID> tid_dist;
        std::uniform_int_distribution<TestSize> nop_dist;

        Address address(URNG& urng);
        Address address_from(URNG& urng,
                             const std::unordered_set<Address>& addresses);
    };

    struct Test {
        size_t generation{1};
        std::vector<ProtoOperation> operations{};
        std::unordered_set<Address> fitaddrs{};
        size_t fitness{0};
        double nd_t{};

        static Test make_random(size_t size, Randomizer& random, URNG& urng);
    };
} // namespace hmv::generator::mcversi

#endif /* __HMV_GENERATOR_MCVERSI_TEST_HPP__ */
