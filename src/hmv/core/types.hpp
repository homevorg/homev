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

#ifndef __HMV_CORE_TYPES_HPP__
#define __HMV_CORE_TYPES_HPP__

#include <cmath>
#include <cstdint>

namespace hmv {
    using Address = uint64_t;
    using AddressCount = uint32_t;
    using BitCount = uint16_t;
    using BlockCount = uint32_t;
    using ByteCount = Address;
    using Data = uint64_t;
    using ProgramCounter = uint32_t;
    using InstanceCount = uint32_t;
    using ThreadID = uint16_t;
    using ThreadCount = ThreadID;
    using TestSize = uint32_t;
    using Time = uint64_t;

    enum class WordSize : uint8_t {
        DWORD = 4,
        QWORD = 8,
    };

    enum class Atomicity : uint8_t {
        OMCA = 0,
        NMCA = 1,
    };

    enum class MCM : uint8_t {
        RELAXED = 0,
        TSO = 1,
    };

    enum class ISA : uint8_t {
        ARM,
        RISCV,
    };

    struct ISATraits {
        ISA architecture{};
        MCM memory_model{};
        Atomicity atomicity{};
    };

    struct MemoryTraits {
        BitCount address_size{64};
        WordSize word_size{WordSize::QWORD};
    };

    struct CacheTraits {
        ByteCount size{};
        ByteCount block_size{};
        BlockCount associativity{};

        constexpr BitCount index_size() const;
        constexpr BitCount offset_size() const;
    };

    inline constexpr BitCount CacheTraits::index_size() const {
        return std::log2(size / (block_size * associativity));
    }

    inline constexpr BitCount CacheTraits::offset_size() const {
        return std::log2(block_size);
    }
} // namespace hmv

#endif /* __HMV_CORE_TYPES_HPP__ */
