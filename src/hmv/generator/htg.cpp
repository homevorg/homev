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

#include "htg.hpp"

#include <algorithm>

#include "hmv/generator/component/nsk_space.hpp"

using hmv::Test;
using hmv::generator::HTG;
using hmv::generator::NSKSpace;

HTG::HTG(MemoryTraits memory_traits,
         CacheTraits cache_traits,
         ThreadCount thread_count,
         coverage::Type coverage_type,
         ExponentRange n_range,
         ExponentRange s_range,
         BitCount abc,
         bool sbc,
         InstanceCount instances,
         const Chaining::Mix& mix,
         Seed seed)
    : address_generator{memory_traits, cache_traits, sbc, seed},
      thread_generator{thread_count, mix, seed}, n_limits{n_range.to_range()},
      s_limits{s_range.to_range()}, abc{abc}, instances{instances}, urng(seed),
      total_coverage{coverage_type} {
    auto n = n_limits.min();
    auto s_min = static_cast<AddressCount>(1u << s_range.min());
    auto s_max = static_cast<AddressCount>(1u << s_range(urng));
    for (auto s = s_min; s <= s_max; s <<= 1) {
        for (auto k = AddressCount{1}; k <= s; k <<= 1) {
            auto point = NSKPoint{n, s, k};
            if (address_generator.feasible({k, point.x()}, abc)) {
                solution.push_back(point);
            }
        }
    }
    traverser.set(solution);
}

std::optional<Test> HTG::operator()() {
    if (auto next = traverser.next()) {
        point = next.value();
        return Test{address_generator(point.s, {point.k, point.x()}, abc),
                    thread_generator(point.n, point.s),
                    instances};
    }
    return {};
}

bool HTG::feedback(const platform::Output& output) {
    visited.insert(point);
    results.insert({point, output.metrics.front()});
    total_coverage.merge(output.metrics.front());
    return explore();
}

bool HTG::explore() {
    if (traverser.empty()) {
        if (!neighbor.empty()) {
            solution = reduce_neighbor();
        }
        neighbor = generate_neighbor(solution);
        if (traverser.empty()) {
            neighbor = generate_neighbor(visited);
        }
        if (traverser.empty()) {
            return true;
        }
    }
    return total_coverage.full();
}

void HTG::apply_perturbation(std::set<NSKPoint>& new_neighbor, NSKPoint point) {
    if (point.n < n_limits.max()) {
        insert_point(new_neighbor, NSKPoint(point.n << 1, point.s, point.k));
    }
    if (point.s < s_limits.max()) {
        insert_point(new_neighbor, NSKPoint(point.n, point.s << 1, point.k));
    }
    if (point.k < point.s) {
        insert_point(new_neighbor, NSKPoint(point.n, point.s, point.k << 1));
    }
    if (point.n > n_limits.min()) {
        insert_point(new_neighbor, NSKPoint(point.n >> 1, point.s, point.k));
    }
    if (point.s > s_limits.min()) {
        insert_point(new_neighbor, NSKPoint(point.n, point.s >> 1, point.k));
    }
    if (point.k > 1) {
        insert_point(new_neighbor, NSKPoint(point.n, point.s, point.k >> 1));
    }
}

template <typename NSKContainer>
NSKSpace HTG::generate_neighbor(const NSKContainer& space) {
    auto new_neighbor = std::set<NSKPoint>{};
    for (auto point : space) {
        apply_perturbation(new_neighbor, point);
    }
    auto sample = NSKSpace{};
    if (!new_neighbor.empty()) {
        auto unexplored = NSKSpace{};
        for (auto point : new_neighbor) {
            if (dist(urng)) {
                sample.push_back(point);
                if (!visited.count(point)) {
                    traverser.insert(point);
                }
            } else if (!visited.count(point)) {
                unexplored.push_back(point);
            }
        }
        if (traverser.empty() && !unexplored.empty()) {
            set_param(index_dist, 0ul, unexplored.size() - 1);
            sample.push_back(*std::next(unexplored.begin(), index_dist(urng)));
            traverser.insert(sample.back());
        }
    }
    return sample;
}

NSKSpace HTG::reduce_neighbor() {
    solution.reserve(solution.size() + neighbor.size());
    for (auto point : neighbor) {
        if (!std::ranges::binary_search(solution, point)) {
            solution.push_back(point);
            if (removed.count(point)) {
                total_coverage.merge(results.at(point));
            }
        }
    }
    std::ranges::shuffle(solution, urng);
    auto reduced_solution = std::set<NSKPoint>{};
    for (auto point : solution) {
        if (total_coverage.affects_value(results.at(point))) {
            reduced_solution.insert(point);
        } else {
            total_coverage.remove(results.at(point));
            removed.insert(point);
        }
    }
    return {reduced_solution.begin(), reduced_solution.end()};
}

void HTG::insert_point(std::set<NSKPoint>& new_neighbor, NSKPoint point) {
    if (address_generator.feasible({point.k, point.x()}, abc)) {
        new_neighbor.insert(point);
    }
}
