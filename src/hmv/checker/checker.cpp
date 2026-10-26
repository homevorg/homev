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

#include "checker.hpp"

#include <cassert>

#include "evch.hpp"
#include "hoch.hpp"
#include "lcheck.hpp"
#include "mrsb.hpp"
#include "srsb.hpp"

using hmv::Time;
using hmv::checker::Checker;
using hmv::checker::Report;

Checker::Checker(Type type,
                 MemoryTraits memory_traits,
                 ThreadCount thread_count,
                 ISATraits traits,
                 const std::filesystem::path& log_dir,
                 bool enable_logger)
    : tag{type} {
    switch (tag) {
    case Type::HOCH:
        model = std::make_unique<Model<HOCH>>(memory_traits,
                                              thread_count,
                                              traits,
                                              log_dir / to_string(tag),
                                              enable_logger);
        break;
    case Type::EVCH:
        model = std::make_unique<Model<EVCH>>(memory_traits,
                                              thread_count,
                                              traits,
                                              log_dir / to_string(tag),
                                              enable_logger);
        break;
    case Type::MRSB:
        model = std::make_unique<Model<MRSB>>(memory_traits,
                                              thread_count,
                                              traits,
                                              log_dir / to_string(tag),
                                              enable_logger);
        break;
    case Type::SRSB:
        model = std::make_unique<Model<SRSB>>(memory_traits,
                                              thread_count,
                                              traits,
                                              log_dir / to_string(tag),
                                              enable_logger);
        break;
    case Type::LCHECK:
        model = std::make_unique<Model<LCHECK>>(memory_traits,
                                                thread_count,
                                                traits,
                                                log_dir / to_string(tag),
                                                enable_logger);
        break;
    }
    assert(model);
}

std::string hmv::checker::to_string(Type type) {
    switch (type) {
    case Type::HOCH:
        return "HOCH";
    case Type::EVCH:
        return "EVCH";
    case Type::MRSB:
        return "MRSB";
    case Type::SRSB:
        return "SRSB";
    case Type::LCHECK:
        return "LCHECK";
    }
    __builtin_unreachable();
}

void Checker::begin_test(const Test& test) {
    model->begin_test(test);
}

void Checker::begin_instance() {
    model->begin_instance();
}

void Checker::begin_thread(ThreadID tid) {
    model->begin_thread(tid);
}

bool Checker::end_thread(ThreadID tid) {
    return model->end_thread(tid);
}

bool Checker::end_instance() {
    return model->end_instance();
}

bool Checker::end_test() {
    return model->end_test();
}

const Report& Checker::report() const {
    return model->report();
}

Time Checker::time() const {
    return model->time();
}
