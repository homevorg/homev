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

#ifndef __HMV_CORE_TRACE_HPP__
#define __HMV_CORE_TRACE_HPP__

#include <filesystem>
#include <fstream>
#include <set>
#include <unordered_map>

#include "event.hpp"
#include "types.hpp"

namespace hmv {
    namespace checker {
        struct Report;
    }

    using TraceSelection =
        std::unordered_map<Operation::Type, std::set<Monitor>>;
    using TraceInstance = std::vector<Event>;

    struct Trace {
        std::vector<TraceInstance> instances;

        Trace filter(const TraceSelection& selection) const;

        void serialize(std::ofstream& file) const;
        void write(const std::filesystem::path& filename) const;
        static Trace deserialize(std::ifstream& file);
        static Trace read(const std::filesystem::path& filename);

        void write_log(const std::filesystem::path& filename,
                       MemoryTraits memory_traits = {}) const;
        void write_log(const std::filesystem::path& filename,
                       const checker::Report& report,
                       MemoryTraits memory_traits = {}) const;
        static Trace read_log(const std::filesystem::path& filename);

     private:
        static constexpr auto sep = ' ';
        static constexpr auto instances_setw = 8;
        static constexpr auto monitor_setw = 17;
        static constexpr auto operation_setw = 9;
        static constexpr auto tid_setw = 3;
        static constexpr auto pc_setw = sizeof(ProgramCounter) * 2;

        void write_log(std::ofstream& file,
                       unsigned address_setw,
                       unsigned value_setw) const;
        static void write_log(std::ofstream& file,
                              const Event& event,
                              unsigned address_setw,
                              unsigned value_setw);
    };
} // namespace hmv

#endif /* __HMV_CORE_TRACE_HPP__ */
