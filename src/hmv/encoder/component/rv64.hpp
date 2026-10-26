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

#ifndef __HMV_ENCODER_RV64_HPP__
#define __HMV_ENCODER_RV64_HPP__

#include <array>

#include "hmv/core/register.hpp"
#include "hmv/core/types.hpp"
#include "hmv/utility/byte.hpp"

namespace hmv::encoder {
    class RV64 {
     public:
        enum ArchRegister {
            // parameter and result registers
            A0 = 10,
            A1 = 11,
            A2 = 12,
            A3 = 13,
            A4 = 14,
            A5 = 15,
            A6 = 16,
            A7 = 17,
            // temporary registers
            T0 = 5,
            T1 = 6,
            T2 = 7,
            T3 = 28,
            T4 = 29,
            T5 = 30,
            T6 = 31,
            // zero register
            ZERO = 0,
        };
        using Instruction = uint32_t;

        constexpr RV64(MemoryTraits traits);
        ByteSequence encode(const std::vector<Address>& addresses) const;
        void encode_add(ByteSequence& sequence,
                        ArchRegister dest,
                        ArchRegister src1,
                        ArchRegister src2) const;
        void encode_exor(ByteSequence& sequence,
                         ArchRegister dest,
                         ArchRegister src1,
                         ArchRegister src2) const;
        void encode_fence(ByteSequence& sequence) const;
        void encode_nop(ByteSequence& sequence, TestSize amount) const;
        void encode_ret(ByteSequence& sequence) const;
        void encode_load(ByteSequence& sequence,
                         ArchRegister dest,
                         ArchRegister src1,
                         Address address) const;
        void encode_store(ByteSequence& sequence,
                          ArchRegister src1,
                          ArchRegister src2,
                          Address address) const;
        void encode_move_address(ByteSequence& sequence,
                                 ArchRegister dest,
                                 Address address) const;
        void encode_move_value(ByteSequence& sequence,
                               ArchRegister dest,
                               Data value) const;
        constexpr ByteCount max_operation_size() const;
        static constexpr ArchRegister map_register(Register reg);

     private:
        bool address_is64;
        bool value_is64;
        static constexpr auto registers =
            std::array<ArchRegister, Register::count>{ArchRegister::A1,
                                                      ArchRegister::A2,
                                                      ArchRegister::A3,
                                                      ArchRegister::A4,
                                                      ArchRegister::T1,
                                                      ArchRegister::T2,
                                                      ArchRegister::T3,
                                                      ArchRegister::T4};

        void base_move(ByteSequence& sequence,
                       ArchRegister dest,
                       uint64_t value,
                       bool is64) const;
    };

    inline constexpr RV64::RV64(MemoryTraits traits)
        : address_is64{traits.address_size > 32},
          value_is64{traits.word_size > WordSize::DWORD} {}

    inline constexpr ByteCount RV64::max_operation_size() const {
        return sizeof(Instruction)
               * (4 + (address_is64 ? 7 : 3) + (value_is64 ? 8 : 4));
    }

    inline constexpr RV64::ArchRegister RV64::map_register(Register reg) {
        return registers[reg];
    }

} // namespace hmv::encoder

#endif /* __HMV_ENCODER_RV64_HPP__ */
