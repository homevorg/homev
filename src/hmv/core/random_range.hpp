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

#ifndef __HMV_CORE_RANDOM_RANGE_HPP__
#define __HMV_CORE_RANDOM_RANGE_HPP__

#include <algorithm>
#include <random>

#include "hmv/utility/math.hpp"
#include "random.hpp"
#include "types.hpp"

namespace hmv {
    class Range;

    class ExponentRange {
     public:
        constexpr ExponentRange(BitCount min, BitCount max);
        constexpr BitCount operator()(URNG& urng);
        constexpr BitCount min() const;
        constexpr BitCount max() const;
        constexpr std::vector<uint32_t> list_values() const;
        constexpr BitCount size() const;
        constexpr Range to_range() const;

     private:
        std::uniform_int_distribution<BitCount> dist;
    };

    class Range {
     public:
        constexpr Range(uint32_t min, uint32_t max);
        constexpr uint32_t operator()(URNG& urng);
        constexpr uint32_t min() const;
        constexpr uint32_t max() const;
        constexpr bool is_power2() const;
        constexpr uint32_t size() const;
        constexpr ExponentRange to_exponent_range() const;

     private:
        std::uniform_int_distribution<uint32_t> dist;
    };

    inline constexpr ExponentRange::ExponentRange(BitCount min, BitCount max)
        : dist{std::min(min, max), std::max(min, max)} {}

    inline constexpr BitCount ExponentRange::operator()(URNG& urng) {
        return dist(urng);
    }

    inline constexpr BitCount ExponentRange::min() const {
        return dist.a();
    }

    inline constexpr BitCount ExponentRange::max() const {
        return dist.b();
    }

    inline constexpr std::vector<uint32_t> ExponentRange::list_values() const {
        auto values = std::vector<uint32_t>{};
        values.resize(dist.b() - dist.a());
        for (auto i = dist.a(); i <= dist.b(); ++i) {
            values.push_back(1 << i);
        }
        return values;
    }

    inline constexpr BitCount ExponentRange::size() const {
        return max() - min() + 1;
    }

    inline constexpr Range ExponentRange::to_range() const {
        return Range{1u << dist.a(), 1u << dist.b()};
    }

    inline constexpr Range::Range(uint32_t min, uint32_t max)
        : dist{std::min(min, max), std::max(min, max)} {}

    inline constexpr uint32_t Range::operator()(URNG& urng) {
        return dist(urng);
    }

    inline constexpr uint32_t Range::min() const {
        return dist.a();
    }

    inline constexpr uint32_t Range::max() const {
        return dist.b();
    }

    inline constexpr bool Range::is_power2() const {
        auto min = dist.a();
        auto max = dist.b();
        return (min != 0 && ((min & (min - 1)) == 0))
               && (max != 0 && ((max & (max - 1)) == 0));
    }

    inline constexpr uint32_t Range::size() const {
        return max() - min() + 1;
    }

    inline constexpr ExponentRange Range::to_exponent_range() const {
        return {static_cast<BitCount>(fast_uint_log2(dist.a())),
                static_cast<BitCount>(fast_uint_log2(dist.b()))};
    }
} // namespace hmv

#endif /* __HMV_CORE_RANDOM_RANGE_HPP__ */
