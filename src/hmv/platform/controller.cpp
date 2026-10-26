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

#include "controller.hpp"

#include <cassert>
#include <filesystem>

#include "hmv/core/test.hpp"
#include "traits.hpp"

using hmv::ByteSequence;
using hmv::InstanceCount;
using hmv::platform::Controller;

Controller::Controller(MemoryTraits memory_traits,
                       ThreadCount thread_count,
                       ISATraits traits,
                       const std::vector<checker::Type>& checker_types,
                       const std::vector<coverage::Type>& coverage_types,
                       const std::vector<size_t>& coverage_totals,
                       const TraceSelection& trace_selection,
                       InstanceCount warmup_instances,
                       TestSize max_thread_size,
                       bool details)
    : memory_traits{memory_traits}, tracer{trace_selection},
      warmup_instances{warmup_instances}, max_thread_size{max_thread_size},
      details{details}, encoder{traits.architecture, memory_traits},
      executing_thread(thread_count, false), warmup_count{warmup_instances} {
    assert(coverage_types.size() == coverage_totals.size());
    for (auto type : checker_types) {
        checkers.emplace_back(type,
                              memory_traits,
                              thread_count,
                              traits,
                              Traits::output_dir,
                              details);
    }
    if (std::filesystem::exists(Traits::output_file)) {
        output = Output::read(Traits::output_file);
        reset();
    } else {
        for (auto i = 0u; i < coverage_types.size(); ++i) {
            output.metrics.emplace_back(coverage_types[i], coverage_totals[i]);
            output.aggregated_metrics.emplace_back(coverage_types[i],
                                                   coverage_totals[i]);
        }
    }
}

Controller::~Controller() {
    quiesce();
}

void Controller::begin_test() {
    if (!executing_test) {
        test_time_counter.start();
        for (auto& checker : checkers) {
            checker.begin_test(test);
        }
        executing_test = true;
    }
}

bool Controller::begin_instance() {
    if (warmup_count == 0 && executing_test && !executing_instance) {
        for (auto& checker : checkers) {
            checker.begin_instance();
        }
        tracer.start();
        executing_instance = true;
        return true;
    }
    return false;
}

bool Controller::begin_thread(ThreadID tid) {
    if (executing_instance && !executing_thread[tid]) {
        for (auto& checker : checkers) {
            checker.begin_thread(tid);
        }
        executing_thread[tid] = true;
        return true;
    }
    return false;
}

InstanceCount Controller::load_test() {
    test = hmv::Test::read(Traits::test_file);
    assert(test.threads.size() == executing_thread.size());
    addresses.insert(test.addresses.begin(), test.addresses.end());
    return test.instances ? test.instances + warmup_instances : 0;
}

ByteSequence Controller::encoded_addresses() const {
    return encoder.encode(test.addresses);
}

std::vector<ByteSequence> Controller::encoded_threads() {
    return encoder.encode(test);
}

void Controller::push_transition(const std::string& fsm,
                                 const std::string& state,
                                 const std::string& event,
                                 const std::string& next_state,
                                 Address line_address,
                                 unsigned instance_id) {
    if (executing_instance) {
        for (auto& metric : output.metrics) {
            metric.push(fsm, state, event, line_address, instance_id);
        }
    }
}

bool Controller::quiesce() {
    constexpr auto empty_callback = [](const checker::Checker& checker,
                                       ThreadID tid) {};
    auto interrupted = false;
    for (auto i = 0u; i < executing_thread.size(); ++i) {
        if (executing_thread[i]) {
            end_thread(i, empty_callback);
            interrupted = true;
        }
    }
    if (executing_instance) {
        end_instance(empty_callback);
        interrupted = true;
    }
    if (executing_test) {
        end_test(empty_callback);
        interrupted = true;
    }
    return interrupted;
}

void Controller::reset() {
    addresses.clear();
    output.test_time = 0;
    output.warmup_time = 0;
    output.checkers.clear();
    for (auto& metric : output.metrics) {
        metric.reset();
    }
    test_time_counter.reset();
    warmup_count = warmup_instances;
}

void Controller::update_output() {
    output.trace = tracer.flush();
    output.test_time = test_time_counter.nanoseconds();
    for (const auto& checker : checkers) {
        output.checkers.push_back(
            {checker.type(), checker.report().code, checker.time()});
    }
    for (auto i = size_t{0}; i < output.metrics.size(); ++i) {
        output.aggregated_metrics[i].merge(output.metrics[i]);
    }
    output.write(Traits::output_file);
}
