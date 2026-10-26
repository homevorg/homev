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

#ifndef __HTS_ARCH_SYSCALL_HPP__
#define __HTS_ARCH_SYSCALL_HPP__

#if defined(__aarch64__)
// ==================================== ARM ====================================
#define SVC(...) asm volatile("svc 0" : "=r"(x0) : __VA_ARGS__ : "memory", "cc")

namespace hts {
    inline long syscall(long n) {
        register long x8 asm("x8") = n;
        register long x0 asm("x0");
        SVC("r"(x8));
        return x0;
    }

    inline long syscall(long n, long a) {
        register long x8 asm("x8") = n;
        register long x0 asm("x0") = a;
        SVC("r"(x8), "0"(x0));
        return x0;
    }

    inline long syscall(long n, long a, long b) {
        register long x8 asm("x8") = n;
        register long x0 asm("x0") = a;
        register long x1 asm("x1") = b;
        SVC("r"(x8), "0"(x0), "r"(x1));
        return x0;
    }

    inline long syscall(long n, long a, long b, long c) {
        register long x8 asm("x8") = n;
        register long x0 asm("x0") = a;
        register long x1 asm("x1") = b;
        register long x2 asm("x2") = c;
        SVC("r"(x8), "0"(x0), "r"(x1), "r"(x2));
        return x0;
    }

    inline long syscall(long n, long a, long b, long c, long d) {
        register long x8 asm("x8") = n;
        register long x0 asm("x0") = a;
        register long x1 asm("x1") = b;
        register long x2 asm("x2") = c;
        register long x3 asm("x3") = d;
        SVC("r"(x8), "0"(x0), "r"(x1), "r"(x2), "r"(x3));
        return x0;
    }

    inline long syscall(long n, long a, long b, long c, long d, long e) {
        register long x8 asm("x8") = n;
        register long x0 asm("x0") = a;
        register long x1 asm("x1") = b;
        register long x2 asm("x2") = c;
        register long x3 asm("x3") = d;
        register long x4 asm("x4") = e;
        SVC("r"(x8), "0"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x4));
        return x0;
    }

    inline long syscall(
        long n, long a, long b, long c, long d, long e, long f) {
        register long x8 asm("x8") = n;
        register long x0 asm("x0") = a;
        register long x1 asm("x1") = b;
        register long x2 asm("x2") = c;
        register long x3 asm("x3") = d;
        register long x4 asm("x4") = e;
        register long x5 asm("x5") = f;
        SVC("r"(x8), "0"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5));
        return x0;
    }
} // namespace hts

#elif defined(__riscv)
// ================================== RISC-V ===================================
#define ECALL(...)                                                             \
    asm volatile("ecall" : "=r"(a0) : __VA_ARGS__ : "memory", "cc")

namespace hts {
    inline long syscall(long n) {
        register long a7 asm("a7") = n;
        register long a0 asm("a0");
        ECALL("r"(a7));
        return a0;
    }

    inline long syscall(long n, long a) {
        register long a7 asm("a7") = n;
        register long a0 asm("a0") = a;
        ECALL("r"(a7), "0"(a0));
        return a0;
    }

    inline long syscall(long n, long a, long b) {
        register long a7 asm("a7") = n;
        register long a0 asm("a0") = a;
        register long a1 asm("a1") = b;
        ECALL("r"(a7), "0"(a0), "r"(a1));
        return a0;
    }

    inline long syscall(long n, long a, long b, long c) {
        register long a7 asm("a7") = n;
        register long a0 asm("a0") = a;
        register long a1 asm("a1") = b;
        register long a2 asm("a2") = c;
        ECALL("r"(a7), "0"(a0), "r"(a1), "r"(a2));
        return a0;
    }

    inline long syscall(long n, long a, long b, long c, long d) {
        register long a7 asm("a7") = n;
        register long a0 asm("a0") = a;
        register long a1 asm("a1") = b;
        register long a2 asm("a2") = c;
        register long a3 asm("a3") = d;
        ECALL("r"(a7), "0"(a0), "r"(a1), "r"(a2), "r"(a3));
        return a0;
    }

    inline long syscall(long n, long a, long b, long c, long d, long e) {
        register long a7 asm("a7") = n;
        register long a0 asm("a0") = a;
        register long a1 asm("a1") = b;
        register long a2 asm("a2") = c;
        register long a3 asm("a3") = d;
        register long a4 asm("a4") = e;
        ECALL("r"(a7), "0"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4));
        return a0;
    }

    inline long syscall(
        long n, long a, long b, long c, long d, long e, long f) {
        register long a7 asm("a7") = n;
        register long a0 asm("a0") = a;
        register long a1 asm("a1") = b;
        register long a2 asm("a2") = c;
        register long a3 asm("a3") = d;
        register long a4 asm("a4") = e;
        register long a5 asm("a5") = f;
        ECALL("r"(a7), "0"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5));
        return a0;
    }
} // namespace hts
#else
// generic fallback (requires stdlib)
#include <sys/syscall.h>
#include <unistd.h>
#endif
#endif /* __HTS_ARCH_SYSCALL_HPP__ */
