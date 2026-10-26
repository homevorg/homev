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

#ifndef __HMV_GENERATOR_RANDOM_ADDRESS_HPP__
#define __HMV_GENERATOR_RANDOM_ADDRESS_HPP__

#include <random>
#include <vector>

#include "hmv/core/random.hpp"
#include "hmv/core/random_range.hpp"
#include "hmv/core/types.hpp"

namespace hmv::generator {
    class RandomAddress {
     public:
        RandomAddress(MemoryTraits memory_traits,
                      Seed seed = std::random_device{}());
        std::vector<Address> operator()(AddressCount s);
        std::vector<Address> operator()(Range& range);

     private:
        BitCount alignment;
        URNG urng;
        std::uniform_int_distribution<AddressCount> dist;
    };
} // namespace hmv::generator

#endif /* __HMV_GENERATOR_RANDOM_ADDRESS_HPP__ */
