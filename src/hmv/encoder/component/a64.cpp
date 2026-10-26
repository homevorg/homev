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

#include "a64.hpp"

using hmv::ByteSequence;
using hmv::encoder::A64;

namespace {
    enum Extend {
        UXTB = 0b000,
        UXTH = 0b001,
        UXTW = 0b010,
        UXTX = 0b011,
        SXTB = 0b100,
        SXTH = 0b101,
        SXTW = 0b110,
        SXTX = 0b111,
    };

    enum Shift {
        LSL = 0b00,
        LSR = 0b01,
        ASR = 0b10,
        ROR = 0b11,
    };

    enum HalfWord {
        FIRST = 0,
        SECOND = 1,
        THIRD = 2,
        FOURTH = 3,
    };

    constexpr auto nop = 0xd503201f;
    constexpr auto ret = 0xd65f03c0;
    constexpr auto shift = std::array{0, 16, 32, 48};
    constexpr auto max_value = std::array{
        0ul,
        (1ul << shift[HalfWord::SECOND]) - 1,
        (1ul << shift[HalfWord::THIRD]) - 1,
        (1ul << shift[HalfWord::FOURTH]) - 1,
    };

    constexpr uint16_t to_half_word(hmv::Data value, HalfWord hw) {
        return value >> shift[hw];
    }

    constexpr A64::Instruction addi(A64::ArchRegister rd,
                                    A64::ArchRegister rn,
                                    uint16_t imm12,
                                    bool shift = false,
                                    bool is64 = true) {
        return is64 << 31 | 0b00100010 << 23 | shift << 22 | imm12 << 10
               | rn << 5 | rd;
    }

    constexpr A64::Instruction add(A64::ArchRegister rd,
                                   A64::ArchRegister rn,
                                   A64::ArchRegister rm,
                                   Shift shift = LSL,
                                   uint8_t amount = 0,
                                   bool is64 = true) {
        return is64 << 31 | 0b0001011 << 24 | shift << 22 | rm << 16
               | (amount & 0x3) << 10 | rn << 5 | rd;
    }

    constexpr A64::Instruction eor(A64::ArchRegister rd,
                                   A64::ArchRegister rn,
                                   A64::ArchRegister rm,
                                   Shift shift = LSL,
                                   uint8_t amount = 0,
                                   bool is64 = true) {
        return is64 << 31 | 0b1001010 << 24 | shift << 22 | rm << 16
               | (amount & 0x3f) << 10 | rn << 5 | rd;
    }

    constexpr A64::Instruction dmb(uint8_t crm = 0b1111) {
        return 0b11010101000000110011000010111111 | crm << 8;
    }

    constexpr A64::Instruction mov(A64::ArchRegister rd,
                                   uint16_t imm16,
                                   HalfWord hw = HalfWord::FIRST,
                                   bool keep = false,
                                   bool is64 = true) {
        return is64 << 31 | keep << 29 | 0b10100101 << 23 | hw << 21
               | imm16 << 5 | rd;
    }

    constexpr A64::Instruction movk(A64::ArchRegister rd,
                                    uint64_t value,
                                    HalfWord hw = HalfWord::FIRST,
                                    bool is64 = true) {
        return mov(rd, to_half_word(value, hw), hw, true, is64);
    }

    constexpr A64::Instruction movz(A64::ArchRegister rd,
                                    uint64_t value,
                                    HalfWord hw = HalfWord::FIRST,
                                    bool is64 = true) {
        return mov(rd, to_half_word(value, hw), hw, false, is64);
    }

    constexpr A64::Instruction mem_access(bool is_load,
                                          A64::ArchRegister rt,
                                          A64::ArchRegister rn,
                                          uint16_t imm12 = 0,
                                          bool is64 = true) {
        return is64 << 30 | 0b101110010 << 23 | is_load << 22
               | (imm12 & 0xfff) << 10 | rn << 5 | rt;
    }

    constexpr A64::Instruction mem_access(bool is_load,
                                          A64::ArchRegister rt,
                                          A64::ArchRegister rn,
                                          A64::ArchRegister rm,
                                          Extend option = UXTX,
                                          bool amount = false,
                                          bool is64 = true) {
        return is64 << 30 | 0b10111000001 << 21 | is_load << 22 | rm << 16
               | option << 13 | amount << 12 | 0b10 << 10 | rn << 5 | rt;
    }

    constexpr A64::Instruction ldr(A64::ArchRegister rt,
                                   A64::ArchRegister rn,
                                   uint16_t imm12 = 0,
                                   bool is64 = true) {
        return mem_access(true, rt, rn, imm12, is64);
    }

    constexpr A64::Instruction ldr(A64::ArchRegister rt,
                                   A64::ArchRegister rn,
                                   A64::ArchRegister rm,
                                   bool is64 = true) {
        return mem_access(true, rt, rn, rm, UXTX, false, is64);
    }

    constexpr A64::Instruction str(A64::ArchRegister rt,
                                   A64::ArchRegister rn,
                                   uint16_t imm12 = 0,
                                   bool is64 = true) {
        return mem_access(false, rt, rn, imm12, is64);
    }

    constexpr A64::Instruction str(A64::ArchRegister rt,
                                   A64::ArchRegister rn,
                                   A64::ArchRegister rm,
                                   bool is64 = true) {
        return mem_access(false, rt, rn, rm, UXTX, false, is64);
    }

    template <typename T>
    void move(hmv::ByteSequence& seq,
              A64::ArchRegister reg,
              T value,
              bool is64) {
        hmv::insert<hmv::Endianness::LE>(seq, movz(reg, value, FIRST));
        hmv::insert<hmv::Endianness::LE>(seq, movk(reg, value, SECOND));
        if (is64) {
            hmv::insert<hmv::Endianness::LE>(seq, movk(reg, value, THIRD));
            hmv::insert<hmv::Endianness::LE>(seq, movk(reg, value, FOURTH));
        }
    }
} // namespace

ByteSequence A64::encode(const std::vector<Address>& addresses) const {
    return to_bytes<Endianness::LE>(addresses);
}

void A64::encode_add(ByteSequence& sequence,
                     ArchRegister dest,
                     ArchRegister src1,
                     ArchRegister src2) const {
    hmv::insert<Endianness::LE>(sequence, add(dest, src1, src2));
}

void A64::encode_exor(ByteSequence& sequence,
                      ArchRegister dest,
                      ArchRegister src1,
                      ArchRegister src2) const {
    hmv::insert<Endianness::LE>(sequence, eor(dest, src1, src2));
}

void A64::encode_fence(ByteSequence& sequence) const {
    hmv::insert<Endianness::LE>(sequence, dmb());
}

void A64::encode_nop(ByteSequence& sequence, TestSize amount) const {
    for (auto i = 0u; i < amount; ++i) {
        insert<Endianness::LE>(sequence, nop);
    }
}

void A64::encode_ret(ByteSequence& sequence) const {
    insert<Endianness::LE>(sequence, ret);
}

void A64::encode_load(ByteSequence& sequence,
                      ArchRegister dest,
                      ArchRegister src1,
                      Address address) const {
    insert<Endianness::LE>(sequence,
                           ldr(dest, src1, A64::ArchRegister::R0, value_is64));
}

void A64::encode_store(ByteSequence& sequence,
                       ArchRegister src1,
                       ArchRegister src2,
                       Address address) const {
    insert<Endianness::LE>(sequence,
                           str(src2, src1, A64::ArchRegister::R0, value_is64));
}

void A64::encode_move_address(ByteSequence& sequence,
                              ArchRegister dest,
                              Address address) const {
    move(sequence, dest, address, address_is64);
}

void A64::encode_move_value(ByteSequence& sequence,
                            ArchRegister dest,
                            Data value) const {
    move(sequence, dest, value, value_is64);
}
