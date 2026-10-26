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

#include "logger.hpp"

using hmv::checker::Logger;

Logger::Logger(MemoryTraits memory_traits,
               const std::filesystem::path& filename,
               ErrorCode no_error_value,
               bool always_write)
    : memory_traits{memory_traits}, filename{filename},
      no_error_value{no_error_value}, always_write{always_write} {}

bool Logger::write(const Report& report) {
    auto write = always_write || report.code != no_error_value;
    if (write) {
        trace.write(filename.replace_extension("bin"));
        filename.replace_extension("log");
        if (report.code != no_error_value) {
            trace.write_log(filename, report, memory_traits);
        } else {
            trace.write_log(filename, memory_traits);
        }
    }
    trace.instances.clear();
    return write;
}
