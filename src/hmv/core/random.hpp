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

#ifndef __HMV_CORE_RANDOM_HPP__
#define __HMV_CORE_RANDOM_HPP__

#include <random>

namespace hmv {
    using URNG = std::mt19937_64;
    using Seed = std::mt19937_64::result_type;

    template <typename T, template <typename> typename Distribution>
    void set_param(Distribution<T>& dist, T a, T b);

    template <typename T, template <typename> typename Distribution>
    inline void set_param(Distribution<T>& dist, T a, T b) {
        dist.param(typename Distribution<T>::param_type(a, b));
    }
} // namespace hmv

#endif /* __HMV_CORE_RANDOM_HPP__ */
