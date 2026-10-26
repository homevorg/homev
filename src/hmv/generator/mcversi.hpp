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

#include <cstdlib>
#include <optional>
#include <tuple>
#include <vector>

#include "component/mcversi/crossover.hpp"
#include "component/mcversi/fitaddrs_collector.hpp"
#include "component/mcversi/test_pool.hpp"
#include "hmv/core/random.hpp"
#include "hmv/core/register_allocator.hpp"
#include "hmv/core/types.hpp"
#include "hmv/platform/output.hpp"

namespace hmv {
    struct Test;
    struct Trace;
} // namespace hmv

namespace hmv::generator {
    // For information about this generator, see:
    // https://www.marcoelver.com/research/mcversi/
    // This is a total adaptation of the original code to operate inside HMV
    class McVerSi {
     public:
        // default values extracted from McVerSi's paper:
        // https://www.marcoelver.com/res/hpca2016-mcversi.pdf
        static constexpr auto default_instances = 10;
        static constexpr auto default_test_size = 1000;
        static constexpr auto default_population_size = 100;
        static constexpr auto default_tournament_size = 2;
        static constexpr auto default_mates = 2;
        static constexpr auto default_mutation_rate = 0.005f;
        static constexpr auto default_p_usel = 0.2f;
        static constexpr auto default_p_bfa = 0.05f;
        static constexpr auto default_chunk_size = 512;
        static constexpr auto default_hole_size = 1024 * 1024;
        static constexpr auto default_nop_max_seq_size = 50;
        // mix originally defined for ARMv7 in
        // https://www.marcoelver.com/files/mcversi_gem5.patch
        static constexpr auto default_mix =
            mcversi::OperationMix{{0.32, 0.24, 0.42, 0.01, 0.01}};

        McVerSi(ThreadCount thread_count,
                ByteCount memory_size,
                ByteCount stride,
                InstanceCount instances = default_instances,
                TestSize test_size = default_test_size,
                size_t population_size = default_population_size,
                size_t tournament_size = default_tournament_size,
                size_t mates = default_mates,
                float mutation_rate = default_mutation_rate,
                double p_usel = default_p_usel,
                double p_bfa = default_p_bfa,
                ByteCount chunk_size = default_chunk_size,
                ByteCount hole_size = default_hole_size,
                TestSize nop_max_seq_size = default_nop_max_seq_size,
                const mcversi::OperationMix& mix = default_mix,
                Seed seed = std::random_device{}());
        std::optional<hmv::Test> operator()();
        void feedback(const platform::Output& output);
        std::tuple<size_t, size_t, AddressCount> test_info() const;
        double nd_t() const;
        size_t fitaddrs_count() const;

     private:
        ThreadCount thread_count;
        InstanceCount instances;
        size_t tournament_size;
        size_t mates;
        mcversi::TestPool pool;
        mcversi::CrossoverMutate crossover_mutate;
        std::vector<mcversi::Test*> selection;
        RegisterAllocator register_allocator;
        size_t test_index{0};
        size_t current_tournament{1};
        AddressCount address_count{0};
        mcversi::FitaddrsCollector fitaddrs_collector{};

        Test extract_test(const mcversi::Test& mcversi_test);
    };

    inline std::tuple<size_t, size_t, AddressCount> McVerSi::test_info() const {
        return {current_tournament,
                selection[test_index]->generation,
                address_count};
    }

    inline double McVerSi::nd_t() const {
        return selection[test_index]->nd_t;
    }

    inline size_t McVerSi::fitaddrs_count() const {
        return selection[test_index]->fitaddrs.size();
    }
} // namespace hmv::generator
