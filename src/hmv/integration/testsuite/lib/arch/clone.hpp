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

#ifndef __HTS_ARCH_GENERIC_HPP__
#define __HTS_ARCH_GENERIC_HPP__

#if defined(__aarch64__)
// ==================================== ARM ====================================
// Define clone function to diverge execution on ret
// Reference: https://nullprogram.com/blog/2023/03/23/
extern "C" long _stack_head_clone(long flags, void* child_stack);
asm(".global _stack_head_clone      \n"
    "_stack_head_clone:             \n"
    "    stp x29, x30, [sp, #-16]!  \n"
    "    mov x8, 0xdc               \n" // SYS_clone = 220
    "    mov x2, 0                  \n"
    "    mov x3, 0                  \n"
    "    mov x4, 0                  \n"
    "    svc 0                      \n"
    "    mov x0, x1                 \n" // move stack pointer to arg0
    "    ldp x29, x30, [sp], #16    \n" // LR gets new entry point from stack
    "    ret                        \n");

namespace hts {
    using entry_point_t = void (*)(void*);

    struct alignas(16) stack_head_t {
        void* frame_pointer;
        entry_point_t entry_point;
    };

    [[gnu::always_inline]] inline long clone(long flags, void* child_stack) {
        return _stack_head_clone(flags, child_stack);
    }

    inline void stack_head_init(stack_head_t* stack_head,
                                entry_point_t entry_point) {
        stack_head->frame_pointer = stack_head;
        stack_head->entry_point = entry_point;
    }
} // namespace hts

#elif defined(__riscv)
// ================================== RISC-V ===================================
extern "C" long _stack_head_clone(long flags, void* child_stack);
asm(".global _stack_head_clone      \n"
    "_stack_head_clone:             \n"
    "    addi sp, sp, -8            \n"
    "    sd ra, 0(sp)               \n"
    "    li a7, 0xdc                \n" // SYS_clone = 220
    "    li a2, 0                   \n"
    "    li a3, 0                   \n"
    "    li a4, 0                   \n"
    "    ecall                      \n"
    "    mv a0, a1                  \n" // move stack pointer to arg0
    "    ld ra, 0(sp)               \n" // LR gets new entry point from stack
    "    addi sp, sp, 8             \n"
    "    ret                        \n");

namespace hts {
    using entry_point_t = void (*)(void*);

    struct alignas(16) stack_head_t {
        entry_point_t entry_point;
    };

    [[gnu::always_inline]] inline long clone(long flags, void* child_stack) {
        return _stack_head_clone(flags, child_stack);
    }

    inline void stack_head_init(stack_head_t* stack_head,
                                entry_point_t entry_point) {
        stack_head->entry_point = entry_point;
    }
} // namespace hts
#else
// generic fallback (requires stdlib)
#include <sched.h>

namespace hts {
    using entry_point_t = int (*)(void*);

    struct alignas(16) stack_head_t {
        entry_point_t entry_point;
    };

    inline void stack_head_init(stack_head_t* stack_head,
                                entry_point_t entry_point) {
        stack_head->entry_point = entry_point;
    }

    inline long clone(long flags, void* child_stack) {
        return ::clone(static_cast<stack_head_t*>(child_stack)->entry_point,
                       child_stack,
                       flags,
                       child_stack);
    }
} // namespace hts
#endif
#endif /* __HTS_ARCH_GENERIC_HPP__ */
