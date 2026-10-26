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

#ifndef __HMV_GENERATOR_HTG_HPP__
#define __HMV_GENERATOR_HTG_HPP__

#include <random>
#include <unordered_map>
#include <unordered_set>

#include "component/biasing.hpp"
#include "component/chaining.hpp"
#include "component/nsk_space.hpp"
#include "hmv/core/random_range.hpp"
#include "hmv/core/test.hpp"
#include "hmv/core/types.hpp"
#include "hmv/coverage/metric.hpp"
#include "hmv/platform/output.hpp"

namespace hmv::generator {
    // Hybrid Test Generator
    class HTG {
        using Traverser = NSKSpaceTraverser<MinKMaxSMinN, MinSMaxKMinN>;

     public:
        static constexpr auto default_mix = Chaining::Mix{{.0, .0, .5, .5}};

        HTG(MemoryTraits memory_traits,
            CacheTraits cache_traits,
            ThreadCount thread_count,
            coverage::Type coverage_type,
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
        Range n_limits;
        Range s_limits;
        BitCount abc;
        InstanceCount instances;
        URNG urng;
        Traverser traverser;
        NSKSpace solution;
        NSKSpace neighbor;
        std::unordered_set<NSKPoint, NSKHash> visited;
        std::unordered_set<NSKPoint, NSKHash> removed;
        std::unordered_map<NSKPoint, coverage::Metric, NSKHash> results;
        coverage::Metric total_coverage;
        NSKPoint point;
        std::bernoulli_distribution dist;
        std::uniform_int_distribution<size_t> index_dist;

        void apply_perturbation(std::set<NSKPoint>& new_neighbor,
                                NSKPoint point);
        bool explore();
        template <typename NSKContainer>
        NSKSpace generate_neighbor(const NSKContainer& space);
        NSKSpace reduce_neighbor();
        void insert_point(std::set<NSKPoint>& new_neighbor, NSKPoint point);
    };

    inline std::tuple<TestSize, AddressCount, AddressCount>
    HTG::constraints() const {
        return {point.n, point.s, point.k};
    }
} // namespace hmv::generator

#endif /* __HMV_GENERATOR_HTG_HPP__ */
