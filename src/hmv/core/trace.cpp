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

#include "trace.hpp"

#include <fstream>
#include <iomanip>
#include <tuple>

#include "hmv/checker/component/report.hpp"
#include "hmv/utility/filesystem.hpp"
#include "hmv/utility/type.hpp"

using hmv::Trace;

namespace {
    std::tuple<unsigned, unsigned> to_setws(hmv::MemoryTraits memory_traits) {
        return {std::ceil(memory_traits.address_size / 4.0),
                to_underlying(memory_traits.word_size) * 2};
    }
} // namespace

Trace Trace::filter(const TraceSelection& selection) const {
    auto trace = Trace{};
    trace.instances.reserve(instances.size());
    for (auto& it : instances) {
        trace.instances.emplace_back();
        for (auto& ev : it) {
            if (selection.count(ev.type)
                && selection.at(ev.type).count(ev.source)) {
                trace.instances.back().push_back(ev);
            }
        }
    }
    return trace;
}

void Trace::serialize(std::ofstream& file) const {
    hmv::serialize(file, instances.size());
    for (auto& events : instances) {
        hmv::serialize(file, events);
    }
}

void Trace::write(const std::filesystem::path& filename) const {
    if (auto file = open_output_file(filename, std::ios::binary)) {
        serialize(file);
    }
}

Trace Trace::deserialize(std::ifstream& file) {
    auto trace = Trace{};
    if (!file.good()) {
        return trace;
    }
    auto count = hmv::deserialize<decltype(trace.instances.size())>(file);
    if (!file.good() || count == 0) {
        return trace;
    }
    trace.instances.reserve(count);
    for (auto i = size_t{0}; i < count; ++i) {
        trace.instances.push_back(hmv::deserialize<std::vector<Event>>(file));
        if (!file.good()) {
            return trace;
        }
    }
    return trace;
}

Trace Trace::read(const std::filesystem::path& filename) {
    if (auto file = open_input_file(filename, std::ios::binary)) {
        return deserialize(file);
    }
    return {};
}

void Trace::write_log(const std::filesystem::path& filename,
                      MemoryTraits memory_traits) const {
    if (auto file = open_output_file(filename)) {
        auto [address_setw, value_setw] = to_setws(memory_traits);
        write_log(file, address_setw, value_setw);
    }
}

void Trace::write_log(const std::filesystem::path& filename,
                      const checker::Report& report,
                      MemoryTraits memory_traits) const {
    if (auto file = open_output_file(filename)) {
        auto [address_setw, value_setw] = to_setws(memory_traits);
        write_log(file, address_setw, value_setw);
        file << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> ERROR DETECTED "
                "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\nResult: "
             << report.code << "\nDescription: " << report.description << '\n';
        if (!report.events.empty()) {
            file << "Offending Events(s):\n";
            for (auto& event : report.events) {
                file << std::right << std::setw(instances_setw) << '*' << sep;
                write_log(file, event, address_setw, value_setw);
            }
        }
    }
}

Trace Trace::read_log(const std::filesystem::path& filename) {
    if (auto file = open_input_file(filename)) {
        auto trace = Trace{};
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        auto instance = InstanceCount{0};
        while (file.good() && file.peek() != '>') {
            file >> instance;
            if (file.eof() || !file.good()) {
                break;
            }
            if (instance == trace.instances.size()) {
                if (!trace.instances.empty()) {
                    trace.instances.back().shrink_to_fit();
                }
                trace.instances.emplace_back();
            } else if (instance > trace.instances.size()) {
                break;
            }
            auto event = Event{};
            auto source_name = std::string{};
            auto type_name = std::string{};
            file >> source_name >> type_name >> std::hex >> event.address
                >> event.value >> std::dec >> event.tid >> std::hex >> event.pc
                >> std::dec >> event.time;
            event.source = to_monitor(source_name);
            event.type = to_operation_type(type_name);
            trace.instances.back().push_back(event);
            file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        return trace;
    }
    return {};
}

void Trace::write_log(std::ofstream& file,
                      unsigned address_setw,
                      unsigned value_setw) const {
    file << "INSTANCE" << sep << std::left << std::setw(monitor_setw)
         << "MONITOR" << sep << "OPERATION" << sep
         << std::setw(address_setw + 2) << "ADDRESS" << sep
         << std::setw(value_setw + 2) << "VALUE" << sep << std::setw(tid_setw)
         << "TID" << sep << std::setw(pc_setw + 2) << "PC" << sep << "TIME\n";
    for (auto i = InstanceCount{0}; i < instances.size(); ++i) {
        for (auto& event : instances[i]) {
            file << std::right << std::setw(instances_setw) << i << sep;
            write_log(file, event, address_setw, value_setw);
        }
    }
}

void Trace::write_log(std::ofstream& file,
                      const Event& event,
                      unsigned address_setw,
                      unsigned value_setw) {
    file << std::left << std::setw(monitor_setw) << event.source << sep
         << std::setw(operation_setw) << event.type << std::hex << std::right
         << std::setfill('0') << sep << "0x" << std::setw(address_setw)
         << event.address << sep << "0x" << std::setw(value_setw) << event.value
         << std::dec << std::setfill(sep) << sep << std::setw(tid_setw)
         << event.tid << std::hex << std::setfill('0') << sep << "0x"
         << std::setw(pc_setw) << event.pc << std::dec << std::left
         << std::setfill(sep) << sep << event.time << '\n';
}
