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

#ifndef __HMV_COVERAGE_FSM_STRUCTURAL_HPP__
#define __HMV_COVERAGE_FSM_STRUCTURAL_HPP__

#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

#include "hmv/core/types.hpp"

namespace hmv::coverage {
    class FSMStructural {
        using EventMap = std::unordered_map<std::string, size_t>;
        using StateMap = std::unordered_map<std::string, EventMap>;
        using Counters = std::unordered_map<std::string, StateMap>;

     public:
        void push(const std::string& fsm,
                  const std::string& state,
                  const std::string& event,
                  Address address,
                  unsigned instance);
        bool affects_value(const FSMStructural& other) const;
        void merge(const FSMStructural& other);
        void remove(const FSMStructural& other);
        void reset();
        size_t value() const;
        void write_csv(const std::filesystem::path& filename) const;
        void serialize(std::ofstream& file) const;
        static FSMStructural deserialize(std::ifstream& file);

     private:
        size_t seen{0};
        Counters counters{};

        void update_seen_transitions(const std::string& fsm,
                                     const std::string& state,
                                     const std::string& event);
    };

    inline size_t FSMStructural::value() const {
        return seen;
    }

    inline void FSMStructural::reset() {
        counters.clear();
        seen = 0;
    }
} // namespace hmv::coverage

#endif /* __HMV_COVERAGE_FSM_STRUCTURAL_HPP__ */
