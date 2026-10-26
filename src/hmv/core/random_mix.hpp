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

#ifndef __HMV_CORE_RANDOM_MIX_HPP__
#define __HMV_CORE_RANDOM_MIX_HPP__

#include <algorithm>
#include <array>
#include <optional>
#include <random>

#include "operation.hpp"
#include "random.hpp"

namespace hmv {
    template <typename ChoiceT, ChoiceT... choices>
    class RatioMix {
        static constexpr auto size = sizeof...(choices);

     public:
        using RatioArray = std::array<float, size>;
        template <typename T>
        using ValueArray = std::array<T, size>;

        constexpr RatioMix(const RatioArray& ratios);
        template <typename T>
        constexpr ValueArray<T> apply(T value) const;

        RatioArray ratios;

     protected:
        RatioMix() = default;
        constexpr void normalize();
    };

    template <typename ChoiceT, ChoiceT... choices>
    class SelectionRatioMix : public RatioMix<ChoiceT, choices...> {
        static constexpr auto size = sizeof...(choices);
        static constexpr auto precision = 10000u;

     public:
        using typename RatioMix<ChoiceT, choices...>::RatioArray;

        constexpr SelectionRatioMix(const RatioArray& ratios);
        constexpr SelectionRatioMix(const RatioMix<ChoiceT, choices...>& mix);
        ChoiceT operator()(URNG& urng);

        using RatioMix<ChoiceT, choices...>::ratios;

     protected:
        constexpr SelectionRatioMix() = default;
        constexpr void update();

     private:
        std::array<unsigned, size> values;
        std::uniform_int_distribution<unsigned> values_dist{0, precision - 1};

        template <unsigned N, ChoiceT value, ChoiceT... others>
        constexpr ChoiceT select(unsigned choice) const;
    };

    template <typename ChoiceT, ChoiceT... choices>
    class RandomRatioMix : public SelectionRatioMix<ChoiceT, choices...> {
        static constexpr auto size = sizeof...(choices);

     public:
        using OptionalRatioArray = std::array<std::optional<float>, size>;
        using typename RatioMix<ChoiceT, choices...>::RatioArray;

        constexpr RandomRatioMix();
        constexpr RandomRatioMix(const OptionalRatioArray& ratios);
        constexpr RandomRatioMix(const RatioMix<ChoiceT, choices...>& mix);
        void randomize(URNG& urng);

        using SelectionRatioMix<ChoiceT, choices...>::ratios;

     private:
        std::array<bool, size> random;
        std::uniform_real_distribution<float> ratios_dist{0.0f, 1.0f};

        constexpr float get(const std::optional<float>& optional) const;
    };

    using OperationMix = RatioMix<Operation::Type,
                                  Operation::LOAD,
                                  Operation::STORE,
                                  Operation::FENCE,
                                  Operation::NOP>;
    using RandomOperationMix = RandomRatioMix<Operation::Type,
                                              Operation::LOAD,
                                              Operation::STORE,
                                              Operation::FENCE,
                                              Operation::NOP>;

    template <typename ChoiceT, ChoiceT... choices>
    inline constexpr RatioMix<ChoiceT, choices...>::RatioMix(
        const RatioArray& ratios)
        : ratios{ratios} {
        normalize();
    }

    template <typename ChoiceT, ChoiceT... choices>
    inline constexpr void RatioMix<ChoiceT, choices...>::normalize() {
        auto total =
            std::max(1.0f, std::accumulate(ratios.begin(), ratios.end(), .0f));
        for (auto i = 0u; i < size; ++i) {
            ratios[i] = ratios[i] / total;
        }
    }

    template <typename ChoiceT, ChoiceT... choices>
    template <typename T>
    inline constexpr RatioMix<ChoiceT, choices...>::ValueArray<T>
    RatioMix<ChoiceT, choices...>::apply(T value) const {
        auto result = ValueArray<T>{};
        for (auto i = 0u; i < size; ++i) {
            result[i] = std::floor(ratios[i] * value);
        }
        return result;
    }

    template <typename ChoiceT, ChoiceT... choices>
    inline constexpr SelectionRatioMix<ChoiceT, choices...>::SelectionRatioMix(
        const RatioArray& ratios)
        : RatioMix<ChoiceT, choices...>(ratios) {
        update();
    }

    template <typename ChoiceT, ChoiceT... choices>
    inline constexpr SelectionRatioMix<ChoiceT, choices...>::SelectionRatioMix(
        const RatioMix<ChoiceT, choices...>& mix)
        : RatioMix<ChoiceT, choices...>(mix) {
        update();
    }

    template <typename ChoiceT, ChoiceT... choices>
    inline ChoiceT
    SelectionRatioMix<ChoiceT, choices...>::operator()(URNG& urng) {
        return select<0, choices...>(values_dist(urng));
    }

    template <typename ChoiceT, ChoiceT... choices>
    constexpr void SelectionRatioMix<ChoiceT, choices...>::update() {
        for (auto i = 0u; i < size; ++i) {
            values[i] = std::floor(ratios[i] * precision);
            if (i > 0) {
                values[i] += values[i - 1];
            }
        }
    }

    template <typename ChoiceT, ChoiceT... choices>
    template <unsigned index, ChoiceT choice, ChoiceT... others>
    inline constexpr ChoiceT
    SelectionRatioMix<ChoiceT, choices...>::select(unsigned value) const {
        if (value < values[index]) {
            return choice;
        }
        if constexpr (sizeof...(others) > 0) {
            return select<index + 1, others...>(value);
        }
        return choice;
    }

    template <typename ChoiceT, ChoiceT... choices>
    inline constexpr RandomRatioMix<ChoiceT, choices...>::RandomRatioMix() {
        random.fill(true);
    }

    template <typename ChoiceT, ChoiceT... choices>
    inline constexpr RandomRatioMix<ChoiceT, choices...>::RandomRatioMix(
        const OptionalRatioArray& optional_ratios) {
        for (auto i = 0u; i < size; ++i) {
            ratios[i] = optional_ratios[i].value_or(0.0f);
            random[i] = !optional_ratios[i];
        }
        RatioMix<ChoiceT, choices...>::normalize();
        SelectionRatioMix<ChoiceT, choices...>::update();
    }

    template <typename ChoiceT, ChoiceT... choices>
    inline constexpr RandomRatioMix<ChoiceT, choices...>::RandomRatioMix(
        const RatioMix<ChoiceT, choices...>& mix)
        : SelectionRatioMix<ChoiceT, choices...>(mix) {}

    template <typename ChoiceT, ChoiceT... choices>
    inline void RandomRatioMix<ChoiceT, choices...>::randomize(URNG& urng) {
        if (auto count = std::accumulate(random.begin(), random.end(), 0)) {
            auto randomized_indices = std::vector<size_t>{};
            randomized_indices.reserve(random.size());
            auto remaining = 1.0f;
            for (auto i = 0u; i < size; ++i) {
                if (!random[i]) {
                    remaining -= ratios[i];
                } else {
                    randomized_indices.push_back(i);
                }
            }
            std::ranges::shuffle(randomized_indices, urng);
            for (auto i : randomized_indices) {
                if (count == 1) {
                    ratios[i] = remaining;
                } else {
                    set_param(ratios_dist, 0.0f, remaining);
                    ratios[i] = ratios_dist(urng);
                }
                remaining -= ratios[i];
                --count;
            }
            SelectionRatioMix<ChoiceT, choices...>::update();
        }
    }
} // namespace hmv

#endif /* __HMV_CORE_RANDOM_MIX_HPP__ */
