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

#ifndef __HMV_CORE_EVENT_HPP__
#define __HMV_CORE_EVENT_HPP__

#include <ostream>
#include <string>

#include "operation.hpp"
#include "types.hpp"

namespace hmv {
    enum Monitor : uint8_t {
        LOCAL_COMMIT = 0,
        REMOTE_COMMIT = 1,
        LOCAL_COMPLETION = 2,
        REMOTE_COMPLETION = 3,
        SQUASH = 4,
        AVAILABILITY = 5,
        MEMORY = 6,
        ENTER_TIME = 7,
        COMMIT_TIME = 8,
        NONE = 255,
    };

    struct Event {
        using Type = Operation::Type;

        Address address{};
        Data value{};
        Type type{Type::NONE};
        Monitor source{Monitor::NONE};
        ThreadID tid{};
        ProgramCounter pc{};
        Time time{};

        constexpr bool operator<(const Event& other) const;
        constexpr bool operator==(const Event& other) const;
        constexpr bool operator!=(const Event& other) const;
        constexpr bool is_load() const;
        constexpr bool is_store() const;
        constexpr bool is_fence() const;
        constexpr bool is_nop() const;
    };

    struct ReadEvent : public Event {
        ReadEvent(Address address,
                  Data value,
                  Monitor source,
                  ThreadID tid,
                  ProgramCounter pc,
                  Time time)
            : Event{address, value, Type::LOAD, source, tid, pc, time} {}
    };

    struct WriteEvent : public Event {
        WriteEvent(Address address,
                   Data value,
                   Monitor source,
                   ThreadID tid,
                   ProgramCounter pc,
                   Time time)
            : Event{address, value, Type::STORE, source, tid, pc, time} {}
    };

    struct FenceEvent : public Event {
        FenceEvent(Monitor source, ThreadID tid, ProgramCounter pc, Time time)
            : Event{0, 0, Type::FENCE, source, tid, pc, time} {}
    };

    template <Monitor monitor>
    struct ReadAt : public ReadEvent {
        ReadAt(Address address,
               Data value,
               ThreadID tid,
               ProgramCounter pc,
               Time time)
            : ReadEvent{address, value, monitor, tid, pc, time} {}
    };

    template <Monitor monitor>
    struct WriteAt : public WriteEvent {
        WriteAt(Address address,
                Data value,
                ThreadID tid,
                ProgramCounter pc,
                Time time)
            : WriteEvent{address, value, monitor, tid, pc, time} {}
    };

    template <Monitor monitor>
    struct FenceAt : public FenceEvent {
        FenceAt(ThreadID tid, ProgramCounter pc, Time time)
            : FenceEvent{monitor, tid, pc, time} {}
    };

    Monitor to_monitor(const std::string& monitor);
    std::string to_string(Monitor monitor);
    std::ostream& operator<<(std::ostream& out, Monitor monitor);

    inline constexpr bool Event::operator<(const Event& other) const {
        if (address != other.address) {
            return address < other.address;
        }
        if (value != other.value) {
            return value < other.value;
        }
        return type < other.type;
    }

    inline constexpr bool Event::operator==(const Event& other) const {
        return type == other.type && address == other.address
               && value == other.value;
    }

    inline constexpr bool Event::operator!=(const Event& other) const {
        return !(operator==(other));
    }

    inline constexpr bool Event::is_load() const {
        return type == Type::LOAD;
    }

    inline constexpr bool Event::is_store() const {
        return type == Type::STORE;
    }

    inline constexpr bool Event::is_fence() const {
        return type == Type::FENCE;
    }

    inline constexpr bool Event::is_nop() const {
        return type == Type::NOP;
    }

} // namespace hmv

#endif /* __HMV_CORE_EVENT_HPP__ */
