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

#include "signals.hpp"

#include <cassert>
#include <cerrno>
#include <ctime>
#include <signal.h>

namespace {
    sigset_t make_mask_and_block() {
        auto mask = sigset_t{};
        sigemptyset(&mask);
        sigaddset(&mask, SIGUSR1);
        [[maybe_unused]] auto result = sigprocmask(SIG_BLOCK, &mask, NULL);
        assert(result == 0);
        return mask;
    }

    int wait(int pid, sigset_t& mask) {
        auto info = siginfo_t{};
        auto timeout = timespec{1, 0};
        while (sigtimedwait(&mask, &info, &timeout) < 0) {
            assert(errno == EAGAIN);
            if (kill(pid, 0) == -1 && errno == ESRCH) {
                return 0;
            }
        }
        return info.si_value.sival_int;
    }
} // namespace

int hmv::platform::wait_continue(int pid) {
    auto mask = make_mask_and_block();
    return wait(pid, mask);
}

int hmv::platform::signal_continue(int pid, int sigval) {
    return sigqueue(pid, SIGUSR1, {sigval});
}

int hmv::platform::signal_continue_and_wait(int pid, int sigval) {
    if (pid < 0) {
        return 1;
    }
    auto mask = make_mask_and_block();
    [[maybe_unused]] auto result = sigqueue(pid, SIGUSR1, {sigval});
    assert(result == 0);
    return wait(pid, mask);
}
