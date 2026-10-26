// Copyright (C) 2019-2026 Marleson Graf

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

#include "event.hpp"

using hmv::Monitor;

Monitor hmv::to_monitor(const std::string& monitor) {
    switch (monitor.size()) {
    case 6:
        if (monitor == "MEMORY") {
            return Monitor::MEMORY;
        } else if (monitor == "SQUASH") {
            return Monitor::SQUASH;
        }
        break;
    case 10:
        if (monitor == "ENTER_TIME") {
            return Monitor::ENTER_TIME;
        }
        break;
    case 11:
        if (monitor == "COMMIT_TIME") {
            return Monitor::COMMIT_TIME;
        }
        break;
    case 12:
        if (monitor == "LOCAL_COMMIT") {
            return Monitor::LOCAL_COMMIT;
        } else if (monitor == "AVAILABILITY") {
            return Monitor::AVAILABILITY;
        }
        break;
    case 13:
        if (monitor == "REMOTE_COMMIT") {
            return Monitor::REMOTE_COMMIT;
        }
        break;
    case 16:
        if (monitor == "LOCAL_COMPLETION") {
            return Monitor::LOCAL_COMPLETION;
        }
        break;
    case 17:
        if (monitor == "REMOTE_COMPLETION") {
            return Monitor::REMOTE_COMPLETION;
        }
        break;
    }
    return Monitor::NONE;
}

std::string hmv::to_string(Monitor monitor) {
    switch (monitor) {
    case Monitor::LOCAL_COMMIT:
        return "LOCAL_COMMIT";
    case Monitor::REMOTE_COMMIT:
        return "REMOTE_COMMIT";
    case Monitor::LOCAL_COMPLETION:
        return "LOCAL_COMPLETION";
    case Monitor::REMOTE_COMPLETION:
        return "REMOTE_COMPLETION";
    case Monitor::SQUASH:
        return "SQUASH";
    case Monitor::AVAILABILITY:
        return "AVAILABILITY";
    case Monitor::MEMORY:
        return "MEMORY";
    case Monitor::ENTER_TIME:
        return "ENTER_TIME";
    case Monitor::COMMIT_TIME:
        return "COMMIT_TIME";
    case Monitor::NONE:
        return "NONE";
    }
    __builtin_unreachable();
}

std::ostream& hmv::operator<<(std::ostream& out, Monitor monitor) {
    return out << to_string(monitor);
}
