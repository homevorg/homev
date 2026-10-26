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

#include "rv64.hpp"

using hmv::ByteSequence;
using hmv::encoder::RV64;

namespace {
    constexpr RV64::Instruction fence = 0x0330000f;
    constexpr RV64::Instruction ret = 0x00008067;

    constexpr RV64::Instruction add(RV64::ArchRegister rd,
                                    RV64::ArchRegister rs1,
                                    RV64::ArchRegister rs2) {
        return rs2 << 20 | rs1 << 15 | rd << 7 | 0x33;
    }

    constexpr RV64::Instruction addi(RV64::ArchRegister rd,
                                     RV64::ArchRegister rs1,
                                     int16_t imm12) {
        return imm12 << 20 | rs1 << 15 | rd << 7 | 0x13;
    }

    constexpr RV64::Instruction lui(RV64::ArchRegister rd, uint32_t imm20) {
        return (imm20 & 0xfffff) << 12 | rd << 7 | 0x37;
    }

    constexpr RV64::Instruction ld(RV64::ArchRegister rd,
                                   RV64::ArchRegister rs1,
                                   int16_t imm12) {
        return imm12 << 20 | rs1 << 15 | rd << 7 | 0x3003;
    }

    constexpr RV64::Instruction lwu(RV64::ArchRegister rd,
                                    RV64::ArchRegister rs1,
                                    int16_t imm12) {
        return imm12 << 20 | rs1 << 15 | rd << 7 | 0x6003;
    }

    constexpr RV64::Instruction sd(RV64::ArchRegister rs1,
                                   RV64::ArchRegister rs2,
                                   int16_t imm12) {
        return (imm12 & 0xfe0) << 20 | rs2 << 20 | rs1 << 15
               | (imm12 & 0x1f) << 7 | 0x3023;
    }

    constexpr RV64::Instruction sw(RV64::ArchRegister rs1,
                                   RV64::ArchRegister rs2,
                                   int16_t imm12) {
        return (imm12 & 0xfe0) << 20 | rs2 << 20 | rs1 << 15
               | (imm12 & 0x1f) << 7 | 0x2023;
    }

    constexpr RV64::Instruction slli(RV64::ArchRegister rd,
                                     RV64::ArchRegister rs1,
                                     uint8_t imm5) {
        return (imm5 & 0x1f) << 20 | rs1 << 15 | rd << 7 | 0x1013;
    }

    constexpr RV64::Instruction exor(RV64::ArchRegister rd,
                                     RV64::ArchRegister rs1,
                                     RV64::ArchRegister rs2) {
        return rs2 << 20 | rs1 << 15 | rd << 7 | 0x4033;
    }
} // namespace

ByteSequence RV64::encode(const std::vector<Address>& addresses) const {
    return to_bytes<Endianness::LE>(addresses);
}

void RV64::encode_add(ByteSequence& sequence,
                      ArchRegister dest,
                      ArchRegister src1,
                      ArchRegister src2) const {
    insert<Endianness::LE>(sequence, add(dest, src1, src2));
}

void RV64::encode_exor(ByteSequence& sequence,
                       ArchRegister dest,
                       ArchRegister src1,
                       ArchRegister src2) const {
    insert<Endianness::LE>(sequence, exor(dest, src1, src2));
}

void RV64::encode_fence(ByteSequence& sequence) const {
    insert<Endianness::LE>(sequence, fence);
}

void RV64::encode_nop(ByteSequence& sequence, TestSize amount) const {
    for (auto i = 0u; i < amount; ++i) {
        insert<Endianness::LE>(sequence, addi(ZERO, ZERO, 0));
    }
}

void RV64::encode_ret(ByteSequence& sequence) const {
    insert<Endianness::LE>(sequence, ret);
}

void RV64::encode_load(ByteSequence& sequence,
                       ArchRegister dest,
                       ArchRegister src1,
                       Address address) const {
    insert<Endianness::LE>(sequence, add(src1, src1, ArchRegister::A0));
    if (value_is64) {
        insert<Endianness::LE>(sequence, ld(dest, src1, address & 0x3ff));
    } else {
        insert<Endianness::LE>(sequence, lwu(dest, src1, address & 0x3ff));
    }
}

void RV64::encode_store(ByteSequence& sequence,
                        ArchRegister src1,
                        ArchRegister src2,
                        Address address) const {
    insert<Endianness::LE>(sequence, add(src1, src1, ArchRegister::A0));
    if (value_is64) {
        insert<Endianness::LE>(sequence, sd(src1, src2, address & 0x3ff));
    } else {
        insert<Endianness::LE>(sequence, sw(src1, src2, address & 0x3ff));
    }
}

void RV64::encode_move_address(ByteSequence& sequence,
                               ArchRegister dest,
                               Address address) const {
    base_move(sequence, dest, address, address_is64);
}

void RV64::encode_move_value(ByteSequence& sequence,
                             ArchRegister dest,
                             Data value) const {
    base_move(sequence, dest, value, value_is64);
    insert<Endianness::LE>(sequence, addi(dest, dest, value & 0x3ff));
}

void RV64::base_move(ByteSequence& sequence,
                     ArchRegister dest,
                     uint64_t value,
                     bool is64) const {
    if (is64) {
        auto high_lower12 = (static_cast<int32_t>(value >> 32) << 20) >> 20;
        auto high_upper20 = ((value >> 32) - high_lower12) >> 12;
        insert<Endianness::LE>(sequence, lui(dest, high_upper20));
        insert<Endianness::LE>(sequence, addi(dest, dest, high_lower12));
        insert<Endianness::LE>(sequence, slli(dest, dest, 11));
        insert<Endianness::LE>(sequence,
                               addi(dest, dest, (value >> 21) & 0x7ff));
        insert<Endianness::LE>(sequence, slli(dest, dest, 11));
        insert<Endianness::LE>(sequence,
                               addi(dest, dest, (value >> 10) & 0x7ff));
    } else {
        auto middle12 = static_cast<int32_t>(value << 10) >> 10;
        auto upper10 = (value - middle12) >> 22;
        insert<Endianness::LE>(sequence, lui(dest, upper10));
        insert<Endianness::LE>(sequence, addi(dest, dest, middle12 >> 10));
    }
    insert<Endianness::LE>(sequence, slli(dest, dest, 10));
}
