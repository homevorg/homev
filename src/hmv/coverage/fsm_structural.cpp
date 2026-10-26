// Copyright (C) 2018-2026 Marleson Graf

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

#include "fsm_structural.hpp"

#include "hmv/utility/filesystem.hpp"

using hmv::coverage::FSMStructural;

void FSMStructural::push(const std::string& fsm,
                         const std::string& state,
                         const std::string& event,
                         hmv::Address address,
                         unsigned instance) {
    update_seen_transitions(fsm, state, event);
    ++counters[fsm][state][event];
}

bool FSMStructural::affects_value(const FSMStructural& other) const {
    for (const auto& [fsm, states] : other.counters) {
        if (!counters.count(fsm)) {
            return true;
        }
        for (const auto& [state, events] : states) {
            if (!counters.at(fsm).count(state)) {
                return true;
            }
            for (const auto& [event, count] : events) {
                if (!counters.at(fsm).at(state).count(event)) {
                    return true;
                }
                if (counters.at(fsm).at(state).at(event) <= count) {
                    return true;
                }
            }
        }
    }
    return false;
}

void FSMStructural::merge(const FSMStructural& other) {
    for (const auto& [fsm, states] : other.counters) {
        for (const auto& [state, events] : states) {
            for (const auto& [event, count] : events) {
                update_seen_transitions(fsm, state, event);
                counters[fsm][state][event] += count;
            }
        }
    }
}

void FSMStructural::remove(const FSMStructural& other) {
    for (const auto& [fsm, states] : other.counters) {
        for (const auto& [state, events] : states) {
            for (const auto& [event, count] : events) {
                if (counters[fsm][state][event] <= count) {
                    --seen;
                    counters[fsm][state].erase(event);
                } else {
                    counters[fsm][state][event] -= count;
                }
            }
        }
    }
}

void FSMStructural::write_csv(const std::filesystem::path& filename) const {
    constexpr auto sep = ',';
    if (auto file = open_output_file(filename)) {
        for (const auto& [fsm, states] : counters) {
            for (const auto& [state, events] : states) {
                for (const auto& [event, count] : events) {
                    file << fsm << sep << state << sep << event << sep << count
                         << '\n';
                }
            }
        }
    }
}

void FSMStructural::serialize(std::ofstream& file) const {
    hmv::serialize(file, seen);
    hmv::serialize(file, counters.size());
    for (const auto& [fsm, states] : counters) {
        hmv::serialize(file, fsm);
        hmv::serialize(file, states.size());
        for (const auto& [state, events] : states) {
            hmv::serialize(file, state);
            hmv::serialize(file, events.size());
            for (const auto& [event, count] : events) {
                hmv::serialize(file, event);
                hmv::serialize(file, count);
            }
        }
    }
}

FSMStructural FSMStructural::deserialize(std::ifstream& file) {
    auto result = FSMStructural{};
    result.seen = hmv::deserialize<size_t>(file);
    auto fsm_count = hmv::deserialize<size_t>(file);
    for (auto i = size_t{0}; i < fsm_count; ++i) {
        auto fsm = hmv::deserialize<std::string>(file);
        auto state_count = hmv::deserialize<size_t>(file);
        for (auto j = size_t{0}; j < state_count; ++j) {
            auto state = hmv::deserialize<std::string>(file);
            auto event_count = hmv::deserialize<size_t>(file);
            for (auto k = size_t{0}; k < event_count; ++k) {
                auto event = hmv::deserialize<std::string>(file);
                auto counter = hmv::deserialize<size_t>(file);
                result.counters[fsm][state][event] = counter;
            }
        }
    }
    return result;
}

void FSMStructural::update_seen_transitions(const std::string& fsm,
                                            const std::string& state,
                                            const std::string& event) {
    if (!counters.count(fsm) || !counters[fsm].count(state)
        || !counters[fsm][state].count(event)) {
        ++seen;
    }
}
