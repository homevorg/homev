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

#include "chaining.hpp"

#include <algorithm>
#include <cassert>
#include <random>

#include "hmv/utility/type.hpp"

using hmv::Address;
using hmv::AddressCount;
using hmv::Operation;
using hmv::TestSize;
using hmv::Thread;
using hmv::ThreadID;
using hmv::generator::Chaining;

namespace {
    template <typename T, typename... Others>
    bool is_different(T&& value, T&& other, Others&&... others) {
        if constexpr (sizeof...(others) > 0) {
            return value != other
                   && is_different(std::forward<T>(value),
                                   std::forward<Others>(others)...);
        }
        return value != other;
    }
} // namespace

Chaining::Chaining(ThreadCount thread_count, const RandomMix& mix, Seed seed)
    : thread_count{thread_count}, randomized_mix{mix}, urng{seed},
      operation_dist{to_underlying(Operation::LOAD),
                     to_underlying(Operation::STORE)} {}

std::vector<Thread> Chaining::operator()(TestSize n,
                                         const Mix& mix,
                                         AddressCount s) {
    randomized_mix = mix;
    return operator()(n, s);
}

std::vector<Thread> Chaining::operator()(Range& range, AddressCount s) {
    return operator()(range(urng), s);
}

std::vector<Thread> Chaining::operator()(TestSize n, AddressCount s) {
    randomized_mix.randomize(urng);
    set_param<AddressCount>(address_dist, 0, s - 1);
    auto context = Context(thread_count, validate_n(n), randomized_mix);
    while (auto k = context.select_category(urng)) {
        switch (k.value()) {
        case CAT0:
            cat0(context);
            break;
        case CAT1:
            cat1(context);
            break;
        case CAT2:
            cat2(context);
            break;
        case CAT3:
            cat3(context);
            break;
        }
    }
    return context.threads;
}

void Chaining::cat0(Context& context) {
    auto [i, count] = context.select_cat0_thread(urng);
    auto a = count > 1 ? select_address() : context.select_address(urng, i);
    while (count-- > 0) {
        if (context.last_is_load(i, a)) {
            context.add_store(i, a);
        } else {
            context.add(select_type(), i, a);
        }
    }
}

void Chaining::cat1(Context& context) {
    auto [i, j] = context.select_cat1_threads(urng);
    auto a = select_address();
    context.add_store(i, a);
    context.add_load(j, a);
    context.add(select_type(), j, a);
}

void Chaining::cat2(Context& context) {
    auto [i, j] = context.select_cat2_threads(urng);
    auto a = select_address();
    context.add(select_type(), i, a);
    context.add_fence(i);
    auto b = select_address(a);
    context.add(select_type(), i, b);
    context.add_sco(select_type(), i, j, b, CAT2);
    j = extend(context, j, a, CAT2);
    context.add_fence(j);
    context.add(select_type(), j, a);
}

void Chaining::cat3(Context& context) {
    auto [i, j, h] = context.select_cat3_threads(urng);
    auto a = select_address();
    context.add_store(h, a);
    context.add_load(i, a);
    context.add_fence(i);
    auto b = select_address(a);
    context.add(select_type(), i, b);
    context.add_sco(select_type(), i, j, b, CAT3);
    j = extend(context, j, a, CAT3);
    context.add_fence(j);
    context.add_load(j, a);
}

ThreadID Chaining::extend(Context& context,
                          ThreadID j,
                          AddressCount a,
                          Category k) {
    if (context.can_extend(j, k)) {
        if (auto count = context.select_count(urng, k)) {
            while (count-- > 0 && context.can_extend(j, k)) {
                auto m = context.select_extend_thread(urng, j, k);
                context.add_fence(j);
                auto c = select_address(a);
                context.add(select_type(), j, c);
                context.add_sco(select_type(), j, m, c, k);
                j = m;
            }
        }
    }
    return j;
}

AddressCount Chaining::select_address() {
    return address_dist(urng);
}

template <typename... Except>
AddressCount Chaining::select_address(Except... except) {
    auto a = select_address();
    while (!is_different(a, except...)) {
        a = select_address();
    }
    return a;
}

Operation::Type Chaining::select_type() {
    return static_cast<Operation::Type>(operation_dist(urng));
}

TestSize Chaining::validate_n(TestSize n) const {
    n = std::max(n, thread_count * 2u);
    return n - (n % thread_count);
}

Chaining::Context::Context(ThreadCount thread_count, TestSize n, const Mix& mix)
    : threads(thread_count), thread_addresses(thread_count),
      thread_sizes(thread_count,
                   static_cast<SizeCategory>(std::min(
                       static_cast<TestSize>(SIZE3), n / thread_count))),
      thread_slots(thread_count, n / thread_count), cat_slots{mix.apply(n)},
      orphan_slots{n} {
    for (auto& thread : threads) {
        thread.reserve(n / thread_count);
    }
    for (unsigned k = CAT0; k <= CAT3; ++k) {
        if (cat_slots[k] >= min_category_sizes[k]) {
            orphan_slots -= cat_slots[k];
        } else {
            cat_slots[k] = 0;
        }
    }
    for (unsigned s = SIZE0; s <= thread_sizes[0]; ++s) {
        threads_ge[s].reserve(thread_count);
        for (auto i = 0u; i < threads.size(); ++i) {
            threads_ge[s].push_back(i);
        }
    }
    desired_categories.reserve(category_count);
    feasible_categories.reserve(category_count);
    for (unsigned i = CAT0; i <= CAT3; ++i) {
        if (cat_slots[i] > 0) {
            desired_categories.push_back(static_cast<Category>(i));
        }
    }
}

void Chaining::Context::add(Operation::Type type, ThreadID i, AddressCount a) {
    if (type == Operation::Type::LOAD) {
        add_load(i, a);
    } else {
        add_store(i, a);
    }
}

void Chaining::Context::add_fence(ThreadID i) {
    threads[i].push_back(Fence{});
    update_thread_size(i);
}

void Chaining::Context::add_load(ThreadID i, AddressCount a) {
    thread_addresses[i][a] = threads[i].size();
    threads[i].push_back(Load{a});
    update_thread_size(i);
}

void Chaining::Context::add_store(ThreadID i, AddressCount a) {
    thread_addresses[i][a] = threads[i].size();
    threads[i].push_back(Store{a, write_value++});
    update_thread_size(i);
}

void Chaining::Context::add_sco(
    Operation::Type type, ThreadID i, ThreadID j, AddressCount b, Category k) {
    auto prev_is_load = threads[i].back().type == Operation::LOAD;
    if (type == Operation::STORE
        || (prev_is_load && (thread_slots[i] < 1 || total_slots(k) < 1))) {
        add_store(j, b);
    } else {
        if (prev_is_load) {
            add_store(i, b);
            update_free_slots(k, 1);
        }
        add_load(j, b);
    }
}

bool Chaining::Context::can_extend(ThreadID j, Category k) const {
    return thread_slots[j] >= SIZE2 && total_slots(k) >= SIZE3
           && threads_ge[SIZE3].size() > (thread_sizes[j] == SIZE3 ? 2 : 1);
}

bool Chaining::Context::feasible(Category k) const {
    if (total_slots(k) >= min_category_sizes[k]) {
        switch (k) {
        case CAT0: {
            return !threads_ge[SIZE1].empty();
        }
        case CAT1: {
            auto ge2 = threads_ge[SIZE2].size();
            return ge2 > 1 || (ge2 == 1 && threads_ge[SIZE1].size() > 1);
        }
        case CAT2: {
            return threads_ge[SIZE3].size() > 1;
        }
        case CAT3: {
            auto ge3 = threads_ge[SIZE3].size();
            return ge3 > 2 || (ge3 == 2 && threads_ge[SIZE1].size() > 2);
        }
        }
    }
    return false;
}

bool Chaining::Context::last_is_load(ThreadID i, AddressCount a) const {
    return thread_addresses[i].count(a)
           && threads[i][thread_addresses[i].at(a)].type == Operation::LOAD;
}

AddressCount Chaining::Context::select_address(std::mt19937_64& urng,
                                               ThreadID i) {
    set_param(index_dist, 0ul, thread_addresses[i].size() - 1);
    return std::next(thread_addresses[i].begin(), index_dist(urng))->first;
}

std::optional<Chaining::Category> Chaining::Context::select_category(
    std::mt19937_64& urng) {
    for (auto k : feasible_categories) {
        if (!feasible(k)) {
            orphan_slots += cat_slots[k];
            cat_slots[k] = 0;
        }
    }
    feasible_categories.clear();
    for (auto k : desired_categories) {
        if (feasible(k)) {
            feasible_categories.push_back(k);
        }
    }
    if (feasible_categories.empty()) {
        for (unsigned k = CAT0; k <= CAT3; ++k) {
            if (feasible(static_cast<Category>(k))) {
                feasible_categories.push_back(static_cast<Category>(k));
            }
        }
    }
    while (!feasible_categories.empty()) {
        set_param(cat_dist, 0ul, feasible_categories.size() - 1);
        auto n = cat_dist(urng);
        auto k = feasible_categories[n];
        if (feasible(k)) {
            return k;
        }
        feasible_categories.erase(std::next(feasible_categories.begin(), n));
    }
    return {};
}

TestSize Chaining::Context::select_count(std::mt19937_64& urng, Category k) {
    auto min = cat_slots[k] < min_category_sizes[k] ? cat_slots[k] : 0u;
    auto max = total_slots(k);
    assert(min <= max);
    set_param(count_dist, min, max);
    return count_dist(urng) / 3;
}

std::tuple<ThreadID, TestSize> Chaining::Context::select_cat0_thread(
    std::mt19937_64& urng) {
    auto i = select_thread(urng, SIZE1);
    auto min = !thread_addresses[i].empty() ? 1u : 2u;
    while (min > total_slots(CAT0)) {
        i = select_thread(urng, SIZE1);
        min = !thread_addresses[i].empty() ? 1ul : 2ul;
    }
    auto max = std::min(thread_slots[i], total_slots(CAT0));
    assert(min <= max);
    set_param(count_dist, min, max);
    auto count = count_dist(urng);
    update_free_slots(CAT0, count);
    return {i, count};
}

std::tuple<ThreadID, ThreadID> Chaining::Context::select_cat1_threads(
    std::mt19937_64& urng) {
    auto j = select_thread(urng, SIZE2);
    auto i = select_thread(urng, SIZE1, j);
    update_free_slots(CAT1, min_category_sizes[CAT1]);
    assert(i != j);
    return {i, j};
}

std::tuple<ThreadID, ThreadID> Chaining::Context::select_cat2_threads(
    std::mt19937_64& urng) {
    auto i = select_thread(urng, SIZE3);
    auto j = select_thread(urng, SIZE3, i);
    assert(i != j);
    update_free_slots(CAT2, min_category_sizes[CAT2]);
    return {i, j};
}

std::tuple<ThreadID, ThreadID, ThreadID> Chaining::Context::select_cat3_threads(
    std::mt19937_64& urng) {
    auto i = select_thread(urng, SIZE3);
    auto j = select_thread(urng, SIZE3, i);
    auto h = select_thread(urng, SIZE1, i, j);
    update_free_slots(CAT3, min_category_sizes[CAT3]);
    assert(i != j && i != h && j != h);
    return {i, j, h};
}

ThreadID Chaining::Context::select_extend_thread(std::mt19937_64& urng,
                                                 ThreadID j,
                                                 Category k) {
    auto m = select_thread(urng, SIZE3, j);
    update_free_slots(k, SIZE3);
    assert(j != m);
    return m;
}

TestSize Chaining::Context::total_slots(Category k) const {
    return cat_slots[k] + orphan_slots;
}

void Chaining::Context::update_free_slots(Category k, TestSize count) {
    if (cat_slots[k] > count) {
        cat_slots[k] -= count;
    } else {
        if (cat_slots[k] != 0) {
            orphan_slots += cat_slots[k];
            cat_slots[k] = 0;
        }
        assert(orphan_slots >= count);
        orphan_slots -= count;
    }
}

void Chaining::Context::update_thread_size(ThreadID i) {
    assert(thread_slots[i] > 0);
    if (--thread_slots[i] < thread_sizes[i]) {
        threads_ge[thread_sizes[i]].erase(
            std::ranges::find(threads_ge[thread_sizes[i]], i));
        thread_sizes[i] = static_cast<SizeCategory>(thread_sizes[i] - 1);
    }
}

ThreadID Chaining::Context::select_thread(std::mt19937_64& urng,
                                          SizeCategory size) {
    assert(!threads_ge[size].empty());
    set_param(index_dist, 0ul, threads_ge[size].size() - 1);
    return threads_ge[size][index_dist(urng)];
}

template <typename... Except>
ThreadID Chaining::Context::select_thread(std::mt19937_64& urng,
                                          SizeCategory size,
                                          Except... except) {
    assert(!threads_ge[size].empty());
    set_param(index_dist, 0ul, threads_ge[size].size() - 1);
    auto i = threads_ge[size][index_dist(urng)];
    if (threads_ge[size].size() > sizeof...(except)) {
        while (!is_different(i, except...)) {
            i = threads_ge[size][index_dist(urng)];
        }
    }
    return i;
}
