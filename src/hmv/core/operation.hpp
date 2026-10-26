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

#ifndef __HMV_CORE_OPERATION_HPP__
#define __HMV_CORE_OPERATION_HPP__

#include <ostream>
#include <vector>

#include "register.hpp"
#include "types.hpp"

namespace hmv {
    struct Operation {
        enum Type : uint8_t {
            LOAD = 0,
            STORE = 1,
            FENCE = 2,
            NOP = 16,
            ADD = 32,
            XOR = 34,
            NONE = 255,
        };

        Data value{};
        AddressCount address{};
        Type type{Type::NONE};
        Register dest{Register::NONE};
        Register src1{Register::NONE};
        Register src2{Register::NONE};

        static constexpr auto mem_type_count = 3u;
    };
    using Thread = std::vector<Operation>;

    struct Load : public Operation {
        constexpr Load(AddressCount address,
                       Register dest = {},
                       Register src1 = {},
                       Register src2 = {})
            : Operation{0, address, LOAD, dest, src1, src2} {}
    };

    struct Store : public Operation {
        constexpr Store(AddressCount address,
                        Data value,
                        Register src1 = {},
                        Register src2 = {},
                        Register dest = {})
            : Operation{value, address, STORE, dest, src1, src2} {}
    };

    struct Fence : public Operation {
        constexpr Fence() : Operation{0, 0, FENCE} {}
    };

    struct Nop : public Operation {
        constexpr Nop(Data amount = 1) : Operation{amount, 0, NOP} {}
    };

    struct Add : public Operation {
        constexpr Add(Register dest,
                      Register src1,
                      Register src2,
                      AddressCount address = {},
                      Data value = {})
            : Operation{value, address, ADD, dest, src1, src2} {}
    };

    struct Xor : public Operation {
        constexpr Xor(Register dest,
                      Register src1,
                      Register src2,
                      AddressCount address = {},
                      Data value = {})
            : Operation{value, address, XOR, dest, src1, src2} {}
    };

    Operation::Type to_operation_type(const std::string& type);
    std::string to_string(Operation::Type type);
    std::ostream& operator<<(std::ostream& out, Operation::Type type);
} // namespace hmv

#endif /* __HMV_CORE_OPERATION_HPP__ */
