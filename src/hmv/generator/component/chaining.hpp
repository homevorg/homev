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

#ifndef __HMV_GENERATOR_CHAINING_HPP__
#define __HMV_GENERATOR_CHAINING_HPP__

#include <array>
#include <optional>
#include <random>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "hmv/core/operation.hpp"
#include "hmv/core/random.hpp"
#include "hmv/core/random_mix.hpp"
#include "hmv/core/random_range.hpp"
#include "hmv/core/types.hpp"

namespace hmv::generator {
    class Chaining {
        using OpType = std::underlying_type_t<Operation::Type>;

     public:
        enum Category : unsigned {
            CAT0 = 0,
            CAT1 = 1,
            CAT2 = 2,
            CAT3 = 3,
        };
        using Mix = RatioMix<Category, CAT0, CAT1, CAT2, CAT3>;
        using RandomMix = RandomRatioMix<Category, CAT0, CAT1, CAT2, CAT3>;

        Chaining(ThreadCount thread_count,
                 const RandomMix& mix = {},
                 Seed seed = std::random_device{}());
        std::vector<Thread> operator()(TestSize n,
                                       const Mix& mix,
                                       AddressCount s);
        std::vector<Thread> operator()(Range& range, AddressCount s);
        std::vector<Thread> operator()(TestSize n, AddressCount s);
        RandomMix mix() const;

     private:
        ThreadCount thread_count;
        RandomMix randomized_mix;
        URNG urng;
        std::uniform_int_distribution<AddressCount> address_dist;
        std::uniform_int_distribution<OpType> operation_dist;

        static constexpr auto category_count = CAT3 + 1;

        class Context {
            enum SizeCategory : TestSize {
                SIZE0 = 0,
                SIZE1 = 1,
                SIZE2 = 2,
                SIZE3 = 3,
            };
            static constexpr auto min_category_sizes =
                std::array<unsigned, category_count>{
                    SIZE1, SIZE1 + SIZE2, 2 * SIZE3, SIZE1 + 2 * SIZE3};
            static constexpr auto size_count = SIZE3 + 1;

         public:
            std::vector<Thread> threads;

            Context(ThreadCount thread_count, TestSize n, const Mix& mix);
            void add(Operation::Type type, ThreadID i, AddressCount a);
            void add_fence(ThreadID i);
            void add_load(ThreadID i, AddressCount a);
            void add_store(ThreadID i, AddressCount a);
            void add_sco(Operation::Type type,
                         ThreadID i,
                         ThreadID j,
                         AddressCount b,
                         Category k);
            bool can_extend(ThreadID j, Category k) const;
            bool feasible(Category k) const;
            bool last_is_load(ThreadID i, AddressCount a) const;
            AddressCount select_address(URNG& urng, ThreadID i);
            std::optional<Category> select_category(URNG& urng);
            TestSize select_count(URNG& urng, Category k);
            std::tuple<ThreadID, TestSize> select_cat0_thread(URNG& urng);
            std::tuple<ThreadID, ThreadID> select_cat1_threads(URNG& urng);
            std::tuple<ThreadID, ThreadID> select_cat2_threads(URNG& urng);
            std::tuple<ThreadID, ThreadID, ThreadID> select_cat3_threads(
                URNG& urng);
            ThreadID select_extend_thread(URNG& urng, ThreadID j, Category k);
            TestSize total_slots(Category k) const;
            void update_free_slots(Category k, TestSize count);
            void update_thread_size(ThreadID i);

         private:
            std::vector<std::unordered_map<Address, TestSize>> thread_addresses;
            std::vector<SizeCategory> thread_sizes;
            std::vector<TestSize> thread_slots;
            std::array<TestSize, category_count> cat_slots;
            TestSize orphan_slots;
            std::vector<Category> desired_categories;
            std::vector<Category> feasible_categories;
            std::array<std::vector<ThreadID>, size_count> threads_ge;
            Data write_value{1};
            std::uniform_int_distribution<size_t> cat_dist;
            std::uniform_int_distribution<TestSize> count_dist;
            std::uniform_int_distribution<size_t> index_dist;

            ThreadID select_thread(URNG& urng, SizeCategory size);
            template <typename... Except>
            ThreadID select_thread(URNG& urng,
                                   SizeCategory size,
                                   Except... except);
        };

        void cat0(Context& context);
        void cat1(Context& context);
        void cat2(Context& context);
        void cat3(Context& context);
        ThreadID extend(Context& context,
                        ThreadID j,
                        AddressCount a,
                        Category k);
        AddressCount select_address();
        template <typename... Except>
        AddressCount select_address(Except... except);
        Operation::Type select_type();
        TestSize validate_n(TestSize n) const;
    };

    inline Chaining::RandomMix Chaining::mix() const {
        return randomized_mix;
    }
} // namespace hmv::generator

#endif /* __HMV_GENERATOR_CHAINING_HPP__ */
