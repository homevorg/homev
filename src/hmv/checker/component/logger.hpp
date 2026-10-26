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

#ifndef __HMV_CHECKER_LOGGER_HPP__
#define __HMV_CHECKER_LOGGER_HPP__

#include <filesystem>

#include "hmv/core/trace.hpp"
#include "hmv/core/types.hpp"
#include "report.hpp"

namespace hmv::checker {
    class Logger {
     public:
        Logger(MemoryTraits memory_traits,
               const std::filesystem::path& filename,
               ErrorCode no_error_value,
               bool always_write = true);
        void add(const Event& event);
        void emplace_instance();
        bool write(const Report& report);
        std::filesystem::path dir() const;

     private:
        MemoryTraits memory_traits;
        std::filesystem::path filename;
        ErrorCode no_error_value;
        bool always_write;
        Trace trace;
    };

    inline void Logger::add(const Event& event) {
        trace.instances.back().push_back(event);
    }

    inline void Logger::emplace_instance() {
        trace.instances.emplace_back();
    }

    inline std::filesystem::path Logger::dir() const {
        return filename.parent_path();
    }
} // namespace hmv::checker
#endif // __HMV_CHECKER_LOGGER_HPP__
