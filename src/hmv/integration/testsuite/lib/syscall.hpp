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

#ifndef __HTS_SYSCALL_HPP__
#define __HTS_SYSCALL_HPP__

#include <cstdint>
#include <linux/futex.h>
#include <sys/syscall.h>

#include "arch/syscall.hpp"

namespace hts {
    inline uint32_t futex_wait(uint32_t* uaddr, uint32_t expected) {
        return syscall(SYS_futex,
                       reinterpret_cast<long>(uaddr),
                       FUTEX_WAIT_PRIVATE,
                       expected,
                       0,
                       0,
                       0);
    }

    inline uint32_t futex_wake(uint32_t* uaddr, uint32_t count) {
        return syscall(SYS_futex,
                       reinterpret_cast<long>(uaddr),
                       FUTEX_WAKE_PRIVATE,
                       count,
                       0,
                       0,
                       0);
    }

    inline void* mmap(void* addr, long length, int prot, int flags) {
        return reinterpret_cast<void*>(syscall(SYS_mmap,
                                               reinterpret_cast<long>(addr),
                                               length,
                                               prot,
                                               flags,
                                               -1,
                                               0));
    }

    inline int munmap(void* addr, long length) {
        return syscall(SYS_munmap, reinterpret_cast<long>(addr), length);
    }

    [[noreturn]] inline void exit(int status) {
        syscall(SYS_exit, status);
        __builtin_unreachable();
    }

    [[noreturn]] inline void exit_group(int status) {
        syscall(SYS_exit_group, status);
        __builtin_unreachable();
    }
} // namespace hts

#endif /* __HTS_SYSCALL_HPP__ */
