// Copyright (C) 2019-2026 Marleson Graf

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

#ifndef __HMV_UTILITY_TYPE_HPP__
#define __HMV_UTILITY_TYPE_HPP__

#include <type_traits>

namespace hmv {
    template <typename T, typename = std::enable_if_t<std::is_enum_v<T>>>
    constexpr std::underlying_type_t<T> to_underlying(T value);

    template <typename T,
              template <typename> typename Definition,
              typename = void>
    struct HasMethod : std::false_type {};

    template <typename T, template <typename> typename Definition>
    struct HasMethod<T, Definition, std::enable_if_t<Definition<T>::value>>
        : std::true_type {};

    template <typename T, typename = void>
    struct IsContainer : std::false_type {};

    template <typename T>
    struct IsContainer<T,
                       std::void_t<decltype(std::declval<T>().data()),
                                   decltype(std::declval<T>().size()),
                                   decltype(std::declval<T>().begin()),
                                   decltype(std::declval<T>().end()),
                                   typename T::size_type,
                                   typename T::value_type>> : std::true_type {};

    template <typename T, typename>
    inline constexpr std::underlying_type_t<T> to_underlying(T value) {
        return static_cast<std::underlying_type_t<T>>(value);
    }
} // namespace hmv

#endif /* __HMV_UTILITY_TYPE_HPP__ */
