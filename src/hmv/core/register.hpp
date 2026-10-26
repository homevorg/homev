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

#ifndef __HMV_CORE_REGISTER_HPP__
#define __HMV_CORE_REGISTER_HPP__

#include <cstdint>

namespace hmv {
    class Register {
     public:
        enum Usage : uint8_t {
            DEFAULT = 0,
            ASSIGN_ADDRESS = 1,
            ASSIGN_VALUE = 2,
            PRESERVE = 4,
        };

        enum Name : uint8_t {
            R1 = 0,
            R2 = 1,
            R3 = 2,
            R4 = 3,
            R5 = 4,
            R6 = 5,
            R7 = 6,
            R8 = 7,
            NONE = 15,
        };
        static constexpr auto count = 8u;

        constexpr Register(Name name = NONE, Usage usage = DEFAULT);
        constexpr operator Name() const;
        constexpr Usage usage() const;
        constexpr void usage(Usage usage);

     private:
        uint8_t value;
    };

    inline constexpr Register::Register(Name name, Usage usage)
        : value{static_cast<uint8_t>((usage << 4) | name)} {}

    inline constexpr Register::operator Name() const {
        return static_cast<Name>(value & 0xf);
    }

    inline constexpr Register::Usage Register::usage() const {
        return static_cast<Usage>(value >> 4);
    }

    inline constexpr void Register::usage(Usage usage) {
        value = static_cast<uint8_t>((usage << 4) | (value & 0xf));
    }
} // namespace hmv

#endif /* __HMV_CORE_REGISTER_HPP__ */
