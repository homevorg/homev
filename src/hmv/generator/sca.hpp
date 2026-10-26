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

#ifndef __HMV_GENERATOR_SCA_HPP__
#define __HMV_GENERATOR_SCA_HPP__

#include <random>

#include "component/biasing.hpp"
#include "component/chaining.hpp"
#include "component/nsk_space.hpp"
#include "hmv/core/test.hpp"
#include "hmv/core/types.hpp"
#include "hmv/platform/output.hpp"

namespace hmv::generator {
    // Steep-Coverage Ascent
    class SCA {
        using Traverser = NSKSpaceTraverser<MinNMinKMaxS, MinNMinSMaxK>;

     public:
        enum class Variant {
            COVERAGE = 0,
            TRADEOFF = 1,
            EFFORT = 2,
        };
        static constexpr auto default_mix = Chaining::Mix{{.0, .0, .5, .5}};

        SCA(Variant variant,
            MemoryTraits memory_traits,
            CacheTraits cache_traits,
            ThreadCount thread_count,
            ExponentRange n_range,
            ExponentRange s_range,
            BitCount abc,
            bool sbc,
            InstanceCount instances,
            const Chaining::Mix& mix = default_mix,
            Seed seed = std::random_device{}());
        std::optional<Test> operator()();
        bool feedback(const platform::Output& output);
        std::tuple<TestSize, AddressCount, AddressCount> constraints() const;

     private:
        Biasing address_generator;
        Chaining thread_generator;
        BitCount abc;
        InstanceCount instances;
        Traverser traverser;
        NSKPoint point{};
    };

    inline std::tuple<TestSize, AddressCount, AddressCount>
    SCA::constraints() const {
        return {point.n, point.s, point.k};
    }
} // namespace hmv::generator

#endif /* __HMV_GENERATOR_SCA_HPP__ */
