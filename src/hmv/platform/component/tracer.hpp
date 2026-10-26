// Copyright (C) 2024-2026 Marleson Graf

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

#ifndef __HMV_PLATFORM_TRACER_HPP__
#define __HMV_PLATFORM_TRACER_HPP__

#include <array>
#include <bitset>

#include "hmv/core/event.hpp"
#include "hmv/core/trace.hpp"

namespace hmv::platform {
    class Tracer {
     public:
        Tracer(const TraceSelection& selection);
        void start();
        void push(const Event& event);
        Trace flush();

     private:
        std::array<std::bitset<256>, Operation::mem_type_count> flags{};
        Trace trace{};
    };

    inline Trace Tracer::flush() {
        auto copy = trace;
        trace.instances.clear();
        return copy;
    }
} // namespace hmv::platform
#endif // __HMV_PLATFORM_TRACER_HPP__
