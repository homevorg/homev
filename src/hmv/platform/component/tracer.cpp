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

#include "tracer.hpp"

#include "hmv/utility/type.hpp"

using hmv::platform::Tracer;

Tracer::Tracer(const TraceSelection& selection) {
    for (auto& [type, monitors] : selection) {
        for (auto& monitor : monitors) {
            flags[to_underlying(type)][monitor] = true;
        }
    }
}

void Tracer::start() {
    trace.instances.emplace_back();
    if (trace.instances.size() > 1) {
        trace.instances.back().reserve(trace.instances.front().size());
    }
}

void Tracer::push(const Event& event) {
    if (flags[to_underlying(event.type)][event.source]) {
        trace.instances.back().push_back(event);
    }
}
