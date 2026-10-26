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

#ifndef __HMV_CORE_REGISTER_ALLOCATOR_HPP__
#define __HMV_CORE_REGISTER_ALLOCATOR_HPP__

#include <random>

#include "operation.hpp"
#include "random.hpp"
#include "register.hpp"

namespace hmv {
    class Test;
} // namespace hmv

namespace hmv {
    class RegisterAllocator {
     public:
        RegisterAllocator() = default;
        RegisterAllocator(Seed seed);
        void initialize(const Test& test);
        template <typename... Registers>
        Register random(Registers... exceptions);
        template <template <typename> typename Set>
        Register random(const Set<Register>& choices);
        template <typename... Registers>
        Operation randomize(Operation op);
        void randomize(Test& test);

     private:
        URNG urng{};
        std::uniform_int_distribution<unsigned> dist{Register::R1,
                                                     Register::R8};
        std::uniform_int_distribution<size_t> index_dist{};
    };

    inline RegisterAllocator::RegisterAllocator(Seed seed) : urng{seed} {}

    template <template <typename> typename Set>
    inline Register RegisterAllocator::random(const Set<Register>& choices) {
        set_param(index_dist, 0ul, choices.size() - 1);
        return *std::next(choices.begin(), index_dist(urng));
    }

    template <typename... Registers>
    inline Register RegisterAllocator::random(Registers... exceptions) {
        auto reg = Register{static_cast<Register::Name>(dist(urng))};
        while ((false || ... || (reg == exceptions))) {
            reg = Register{static_cast<Register::Name>(dist(urng))};
        }
        return reg;
    }

    template <typename... Registers>
    inline Operation RegisterAllocator::randomize(Operation op) {
        switch (op.type) {
        case Operation::LOAD:
            if (op.dest == Register::NONE) {
                op.dest = random();
            }
            if (op.src1 == Register::NONE) {
                op.src1 = random(op.src2);
            }
            break;
        case Operation::STORE:
            if (op.src1 == Register::NONE) {
                op.src1 = random(op.dest, op.src2);
            }
            if (op.src2 == Register::NONE) {
                op.src2 = random(op.dest, op.src1);
            }
            break;
        case Operation::XOR:
            if (op.dest == Register::NONE) {
                op.dest = random(op.src1, op.src2);
            }
            if (op.src1 == Register::NONE) {
                op.src1 = random(op.dest, op.src2);
            }
            if (op.src2 == Register::NONE) {
                op.src2 = random(op.dest, op.src1);
            }
        default:
            break;
        }
        return op;
    }

} // namespace hmv

#endif /* __HMV_CORE_REGISTER_ALLOCATOR_HPP__ */
