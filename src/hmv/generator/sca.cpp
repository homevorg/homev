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

#include "sca.hpp"

using hmv::Test;
using hmv::generator::SCA;

SCA::SCA(Variant variant,
         MemoryTraits memory_traits,
         CacheTraits cache_traits,
         ThreadCount thread_count,
         ExponentRange n_range,
         ExponentRange s_range,
         BitCount abc,
         bool sbc,
         InstanceCount instances,
         const Chaining::Mix& mix,
         Seed seed)
    : address_generator{memory_traits, cache_traits, sbc, seed},
      thread_generator{thread_count, mix, seed}, abc{abc}, instances{instances},
      traverser{variant != Variant::EFFORT} {
    switch (variant) {
    case Variant::COVERAGE:
        for (auto n_exp = n_range.min(); n_exp <= n_range.max(); ++n_exp) {
            auto n = 1u << n_exp;
            for (auto s_exp = s_range.min(); s_exp <= s_range.max(); ++s_exp) {
                auto s = static_cast<BitCount>(1u << s_exp);
                for (auto k = BitCount{1}; k <= s; k <<= 1) {
                    if (address_generator.feasible(
                            {k, static_cast<AddressCount>(s / k)}, abc)) {
                        traverser.insert({n, s, k}, k != 1);
                    }
                }
            }
        }
        break;
    case Variant::TRADEOFF:
        for (auto n_exp = n_range.min(); n_exp <= n_range.max(); ++n_exp) {
            auto n = 1u << n_exp;
            for (auto s_exp = s_range.min(); s_exp <= s_range.max(); ++s_exp) {
                auto s = static_cast<BitCount>(1u << s_exp);
                if (address_generator.feasible({1u, s}, abc)) {
                    traverser.insert({n, s, 1u}, false);
                }
                if (address_generator.feasible({s, 1u}, abc)) {
                    traverser.insert({n, s, s}, s != 1);
                }
            }
        }
        break;
    case Variant::EFFORT:
        for (auto n_exp = n_range.min(); n_exp <= n_range.max(); ++n_exp) {
            auto n = 1u << n_exp;
            for (auto s_exp = s_range.min(); s_exp <= s_range.max(); ++s_exp) {
                auto s = static_cast<BitCount>(1u << s_exp);
                if (address_generator.feasible({1u, s}, abc)) {
                    traverser.insert({n, s, 1u}, false);
                }
            }
        }
        break;
    }
}

std::optional<Test> SCA::operator()() {
    if (auto next = traverser.next()) {
        point = next.value();
        return Test{address_generator(point.s, {point.k, point.x()}, abc),
                    thread_generator(point.n, point.s),
                    instances};
    }
    return {};
}

bool SCA::feedback(const platform::Output& output) {
    return output.aggregated_metrics.front().full();
}
