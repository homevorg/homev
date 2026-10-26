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

#include "metric.hpp"

#include <cassert>

#include "fsm_per_instance.hpp"
#include "fsm_structural.hpp"
#include "hmv/utility/filesystem.hpp"

using hmv::coverage::Metric;

Metric::Metric(Type type, size_t max) : tag{type}, max_value{max} {
    switch (tag) {
    case Type::FSM_STRUCTURAL:
        model = std::make_unique<Model<FSMStructural>>(FSMStructural{});
        break;
    case Type::FSM_PER_INSTANCE:
        model = std::make_unique<Model<FSMPerInstance>>(FSMPerInstance{});
        break;
    }
    assert(model);
}

Metric::Metric(Type type, size_t max, std::ifstream& file)
    : tag{type}, max_value{max} {
    switch (tag) {
    case Type::FSM_STRUCTURAL:
        model = std::make_unique<Model<FSMStructural>>(file);
        break;
    case Type::FSM_PER_INSTANCE:
        model = std::make_unique<Model<FSMPerInstance>>(file);
        break;
    }
    assert(model);
}

Metric::Metric(const Metric& other)
    : tag{other.tag}, max_value{other.max_value}, model{other.model->clone()} {}

Metric& Metric::operator=(const Metric& other) {
    tag = other.tag;
    max_value = other.max_value;
    model = other.model->clone();
    return *this;
}

Metric Metric::clone() const {
    return Metric{*this};
}

void Metric::push(const std::string& fsm,
                  const std::string& state,
                  const std::string& event,
                  hmv::Address address,
                  unsigned instance_id) {
    model->push(fsm, state, event, address, instance_id);
}

bool Metric::affects_value(const Metric& other) const {
    return model->affects_value(other);
}

void Metric::merge(const Metric& other) {
    max_value = std::max(max_value, other.max_value);
    model->merge(other);
}

void Metric::remove(const Metric& other) {
    model->remove(other);
}

void Metric::reset() {
    model->reset();
}

bool Metric::full() const {
    return model->value() >= max_value;
}

size_t Metric::max() const {
    return max_value;
}

double Metric::percentage() const {
    return (static_cast<double>(model->value()) / max_value) * 100;
}

size_t Metric::value() const {
    return model->value();
}

void Metric::serialize(std::ofstream& file) const {
    hmv::serialize(file, tag);
    hmv::serialize(file, max_value);
    model->serialize(file);
}

void Metric::write(const std::filesystem::path& filename) const {
    if (auto file = open_output_file(filename, std::ios::binary)) {
        serialize(file);
    }
}

void Metric::write_csv(const std::filesystem::path& filename) const {
    model->write_csv(filename);
}

Metric Metric::deserialize(std::ifstream& file) {
    return {hmv::deserialize<Type>(file), hmv::deserialize<size_t>(file), file};
}

Metric Metric::read(const std::filesystem::path& filename) {
    if (auto file = open_input_file(filename, std::ios::binary)) {
        return deserialize(file);
    }
    return Metric{Type{}};
}
