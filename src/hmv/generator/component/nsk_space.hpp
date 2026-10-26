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

#ifndef __HMV_GENERATOR_NSK_SPACE_HPP__
#define __HMV_GENERATOR_NSK_SPACE_HPP__

#include <optional>
#include <set>
#include <vector>

#include "hmv/core/types.hpp"

namespace hmv::generator {
    struct NSKPoint {
        TestSize n{};
        AddressCount s{};
        AddressCount k{};

        constexpr auto operator<=>(const NSKPoint& other) const = default;
        constexpr AddressCount x() const;
    };

    struct NSKHash {
        size_t operator()(const NSKPoint& point) const {
            return static_cast<size_t>(point.n)
                   | static_cast<size_t>(point.s) << 48
                   | static_cast<size_t>(point.k) << 32;
        }
    };

    using NSKSpace = std::vector<NSKPoint>;

    template <typename Compare, typename AltCompare>
    class NSKSpaceTraverser {
     public:
        NSKSpaceTraverser(bool alternate = true);
        NSKSpaceTraverser(const NSKSpace& space, bool alternate = true);
        std::optional<NSKPoint> next();
        bool empty() const;
        void insert(const NSKPoint& point, bool both = true);
        void set(const NSKSpace& space);

     private:
        std::set<NSKPoint, Compare> traverse_order{};
        std::set<NSKPoint, AltCompare> alt_traverse_order{};
        bool alternate{};
        bool use_alt{};
    };

    struct MinNMinKMaxS {
        constexpr bool operator()(const NSKPoint& lhs,
                                  const NSKPoint& rhs) const;
    };

    struct MinNMinSMaxK {
        constexpr bool operator()(const NSKPoint& lhs,
                                  const NSKPoint& rhs) const;
    };

    struct MinKMaxSMinN {
        constexpr bool operator()(const NSKPoint& lhs,
                                  const NSKPoint& rhs) const;
    };

    struct MinSMaxKMinN {
        constexpr bool operator()(const NSKPoint& lhs,
                                  const NSKPoint& rhs) const;
    };

    constexpr AddressCount NSKPoint::x() const {
        return s / k;
    }

    template <typename Compare, typename AltCompare>
    inline NSKSpaceTraverser<Compare, AltCompare>::NSKSpaceTraverser(
        bool alternate)
        : alternate{alternate} {}

    template <typename Compare, typename AltCompare>
    inline NSKSpaceTraverser<Compare, AltCompare>::NSKSpaceTraverser(
        const NSKSpace& space, bool alternate)
        : alternate{alternate} {
        set(space);
    }

    template <typename Compare, typename AltCompare>
    inline bool NSKSpaceTraverser<Compare, AltCompare>::empty() const {
        return traverse_order.empty();
    }

    template <typename Compare, typename AltCompare>
    inline void NSKSpaceTraverser<Compare, AltCompare>::insert(
        const NSKPoint& point, bool both) {
        traverse_order.insert(point);
        if (alternate && both) {
            alt_traverse_order.insert(point);
        }
    }

    template <typename Compare, typename AltCompare>
    inline std::optional<NSKPoint>
    NSKSpaceTraverser<Compare, AltCompare>::next() {
        if (use_alt) {
            if (alt_traverse_order.empty()) {
                return {};
            }
            auto point = *alt_traverse_order.begin();
            alt_traverse_order.erase(point);
            traverse_order.erase(point);
            use_alt = false;
            return point;
        }
        if (traverse_order.empty()) {
            return {};
        }
        auto point = *traverse_order.begin();
        traverse_order.erase(point);
        alt_traverse_order.erase(point);
        use_alt = alternate && !alt_traverse_order.empty();
        return point;
    }

    template <typename Compare, typename AltCompare>
    inline void
    NSKSpaceTraverser<Compare, AltCompare>::set(const NSKSpace& space) {
        traverse_order.clear();
        traverse_order.insert(space.begin(), space.end());
        if (alternate) {
            alt_traverse_order.clear();
            alt_traverse_order.insert(space.begin(), space.end());
        }
    }

    inline constexpr bool MinNMinKMaxS::operator()(const NSKPoint& lhs,
                                                   const NSKPoint& rhs) const {
        if (lhs.n != rhs.n) {
            return lhs.n < rhs.n;
        }
        if (lhs.k != rhs.k) {
            return lhs.k < rhs.k;
        }
        return lhs.s > rhs.s;
    }

    inline constexpr bool MinNMinSMaxK::operator()(const NSKPoint& lhs,
                                                   const NSKPoint& rhs) const {
        if (lhs.n != rhs.n) {
            return lhs.n < rhs.n;
        }
        if (lhs.s != rhs.s) {
            return lhs.s < rhs.s;
        }
        return lhs.k > rhs.k;
    }

    inline constexpr bool MinKMaxSMinN::operator()(const NSKPoint& lhs,
                                                   const NSKPoint& rhs) const {
        if (lhs.k != rhs.k) {
            return lhs.k < rhs.k;
        }
        if (lhs.s != rhs.s) {
            return lhs.s > rhs.s;
        }
        return lhs.n < rhs.n;
    }

    inline constexpr bool MinSMaxKMinN::operator()(const NSKPoint& lhs,
                                                   const NSKPoint& rhs) const {
        if (lhs.s != rhs.s) {
            return lhs.s < rhs.s;
        }
        if (lhs.k != rhs.k) {
            return lhs.k > rhs.k;
        }
        return lhs.n < rhs.n;
    }
} // namespace hmv::generator

#endif /* __HMV_GENERATOR_NSK_SPACE_HPP__ */
