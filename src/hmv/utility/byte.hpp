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

#ifndef __HMV_UTILITY_BYTE_HPP__
#define __HMV_UTILITY_BYTE_HPP__

#include <climits>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace hmv {
    using Byte = char;
    template <typename ByteT = char>
    using GenericByteSequence = std::vector<ByteT>;
    using ByteSequence = GenericByteSequence<Byte>;

    enum class Endianness {
        BE,
        LE,
    };

    template <Endianness e = Endianness::LE>
    inline constexpr size_t get_shift(size_t offset, size_t size) {
        return (e == Endianness::LE ? offset : size - offset - 1) * CHAR_BIT;
    }

    template <Endianness e = Endianness::LE,
              typename IntegralT = uintmax_t,
              typename ByteT = Byte>
    inline constexpr ByteT to_byte(IntegralT value, size_t offset) {
        return value >> get_shift<e>(offset, sizeof(IntegralT));
    }

    template <Endianness e = Endianness::LE,
              typename IntegralT = uintmax_t,
              typename ByteT = Byte>
    inline constexpr void insert(GenericByteSequence<ByteT>& seq,
                                 IntegralT value) {
        for (auto i = 0u; i < sizeof(IntegralT); ++i) {
            seq.push_back(to_byte<e, IntegralT, ByteT>(value, i));
        }
    }

    template <Endianness e = Endianness::LE,
              typename IntegralT = uintmax_t,
              typename ByteT = Byte>
    inline constexpr void replace(GenericByteSequence<ByteT>& seq,
                                  IntegralT value,
                                  size_t offset = 0) {
        for (auto i = 0u; i < sizeof(IntegralT); ++i) {
            seq[offset + i] = to_byte<e, IntegralT, ByteT>(value, i);
        }
    }

    template <Endianness e = Endianness::LE,
              typename IntegralT = uintmax_t,
              typename ByteT = Byte,
              template <typename T> typename Container = std::vector>
    inline constexpr GenericByteSequence<ByteT>
    to_bytes(const Container<IntegralT>& values) {
        auto sequence = GenericByteSequence<ByteT>{};
        sequence.reserve(values.size() * sizeof(IntegralT));
        for (auto& value : values) {
            insert<e, IntegralT, ByteT>(sequence, value);
        }
        return sequence;
    }

    template <Endianness e = Endianness::LE,
              typename IntegralT = uintmax_t,
              typename ByteT = Byte>
    inline constexpr GenericByteSequence<ByteT> to_bytes(IntegralT value) {
        auto sequence = ByteSequence{};
        sequence.reserve(sizeof(IntegralT));
        insert<e, IntegralT, ByteT>(sequence, value);
        return sequence;
    }
} // namespace hmv

#endif /* __HMV_UTILITY_BYTE_HPP__ */
