// Copyright (C) 2019-2026 Marleson Graf

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

#include "hmv_controller.hh"

#include <ostream>

#include "hmv/core/trace.hpp"
#include "hmv/core/types.hpp"
#include "hmv/platform/component/signals.hpp"
#include "hmv/platform/controller.hpp"

#include "cpu/thread_context.hh"
#include "debug/HMVController.hh"
#include "enums/HMVChecker.hh"
#include "enums/HMVCoverageMetric.hh"
#include "enums/HMVISA.hh"
#include "hmv/utility/type.hpp"
#include "mem/se_translating_port_proxy.hh"
#include "mem/translating_port_proxy.hh"
#include "sim/bufval.hh"
#include "sim/full_system.hh"
#include "sim/process.hh"
#include "sim/sim_exit.hh"
#include "sim/system.hh"

using gem5::HMVController;
using Monitor = hmv::Monitor;
template <Monitor source>
using FenceAt = hmv::FenceAt<source>;
template <Monitor source>
using ReadAt = hmv::ReadAt<source>;
template <Monitor source>
using WriteAt = hmv::WriteAt<source>;

namespace {
    using gem5::Addr;
    using gem5::ThreadContext;
    using hmv::checker::Checker;

    void end_simulation(Checker& checker, hmv::ThreadID tid) {
        const auto& report = checker.report();
        std::stringstream formatter;
        formatter << hmv::checker::to_string(checker.type()) << " @"
                  << std::right << std::setw(2) << std::setfill('0') << tid
                  << " detected an error: " << report.description;
        gem5::exitSimLoopNow(formatter.str(), report.code);
    }

    std::vector<hmv::checker::Type>
    extract_checkers(const HMVController::Params& p) {
        auto checkers = std::vector<hmv::checker::Type>{};
        checkers.reserve(p.checkers.size());
        for (auto checker : p.checkers) {
            checkers.push_back(static_cast<hmv::checker::Type>(checker));
        }
        return checkers;
    }

    std::vector<hmv::coverage::Type>
    extract_metrics(const HMVController::Params& p) {
        auto metrics = std::vector<hmv::coverage::Type>{};
        metrics.reserve(p.coverage_metrics.size());
        for (auto metric : p.coverage_metrics) {
            metrics.push_back(static_cast<hmv::coverage::Type>(metric));
        }
        return metrics;
    }

    hmv::TraceSelection extract_selection(const HMVController::Params& p) {
        auto convert_selection = [](const std::vector<uint8_t>& selection) {
            auto result = std::set<hmv::Monitor>{};
            for (auto value : selection) {
                result.insert(static_cast<hmv::Monitor>(value));
            }
            return result;
        };
        return {
            {hmv::Operation::LOAD, convert_selection(p.trace_read_sel)},
            {hmv::Operation::STORE, convert_selection(p.trace_write_sel)},
            {hmv::Operation::FENCE, convert_selection(p.trace_fence_sel)}};
    }

    hmv::MCM extract_mcm(gem5::HMVISA architecture) {
        switch (architecture) {
        case gem5::HMVISA::ARM:
        case gem5::HMVISA::RISCV:
            return hmv::MCM::RELAXED;
        default:
            return hmv::MCM::TSO;
        }
    }

    hmv::Atomicity extract_atomicity(gem5::HMVISA architecture) {
        switch (architecture) {
        case gem5::HMVISA::ARM:
        case gem5::HMVISA::RISCV:
        default:
            return hmv::Atomicity::OMCA;
        }
    }

    hmv::ISATraits extract_isa_traits(const HMVController::Params& p) {
        return {static_cast<hmv::ISA>(p.architecture),
                extract_mcm(p.architecture),
                extract_atomicity(p.architecture)};
    }

    std::unique_ptr<gem5::TranslatingPortProxy> make_proxy(ThreadContext* tc) {
        return gem5::FullSystem
                   ? std::make_unique<gem5::TranslatingPortProxy>(tc)
                   : std::make_unique<gem5::SETranslatingPortProxy>(tc);
    }
} // namespace

HMVController::HMVController(const Params& p)
    : ClockedObject(p),
      core{std::make_unique<hmv::platform::Controller>(
          hmv::MemoryTraits{p.address_size,
                            static_cast<hmv::WordSize>(p.word_size)},
          p.thread_count,
          extract_isa_traits(p),
          extract_checkers(p),
          extract_metrics(p),
          p.coverage_totals,
          extract_selection(p),
          p.warmup_instances,
          p.max_thread_size,
          p.details)},
      address_size{p.address_size},
      guest_word_size{hmv::to_underlying(p.word_size)},
      physical_memory_size{p.physical_memory_size},
      shared_memory_size{1u << address_size},
      shared_physical_region{physical_memory_size - shared_memory_size},
      shared_memory_region{p.shared_memory_region},
      code_memory_size{core->max_encoded_thread_size() * p.thread_count},
      active_fault{p.fault}, generator_pid{p.generator_pid},
      thread_barrier{p.thread_count} {
    DPRINTF(HMVController, "Thread count: %d\n", p.thread_count);
    DPRINTF(HMVController,
            "Physical memory size: %d MiB\n",
            physical_memory_size >> 20);
    DPRINTF(HMVController,
            "Shared memory size: %d MiB\n",
            shared_memory_size >> 20);
    DPRINTF(HMVController,
            "Shared memory region (physical): %#x\n",
            shared_physical_region);
    DPRINTF(HMVController,
            "Shared memory region (virtual): %#x\n",
            shared_memory_region);
    DPRINTF(HMVController,
            "Enabled fault: %s\n",
            HMVFaultStrings[static_cast<int>(active_fault)]);
    DPRINTF(HMVController, "Generator PID: %d\n", p.generator_pid);
    DPRINTF(HMVController, "Checkers:\n");
    for (auto checker : p.checkers) {
        DPRINTF(HMVController,
                "    %s\n",
                HMVCheckerStrings[static_cast<int>(checker)]);
    }
    DPRINTF(HMVController, "Coverage metrics:\n");
    for (auto metric : p.coverage_metrics) {
        DPRINTF(HMVController,
                "    %s\n",
                HMVCoverageMetricStrings[static_cast<int>(metric)]);
    }
    DPRINTF(HMVController, "Coverage totals:\n");
    for (auto total : p.coverage_totals) {
        DPRINTF(HMVController, "    %d\n", total);
    }
    registerExitCallback([this]() { destroy(); });
}

void HMVController::initState() {
    for (auto system : System::systemList) {
        if (system->hmvController == this) {
            owner_system = system;
            break;
        }
    }
    assert(owner_system != nullptr);
    owner_system->threads[0]->getProcessPtr()->map(
        shared_memory_region, shared_physical_region, shared_memory_size);
}

uint64_t HMVController::begin_program(uint64_t word_size) {
    guest_byte_order = owner_system->getGuestByteOrder();
    DPRINTF(HMVController,
            "Begin program called (byte order: %s, word size: %d)\n",
            ByteOrderStrings[static_cast<int>(guest_byte_order)],
            word_size);
    assert(guest_word_size == word_size);
    return shared_memory_region;
}

void HMVController::begin_test() {
    DPRINTF(HMVController, "Begin test called\n");
    core->begin_test();
}

void HMVController::begin_instance() {
    if (core->begin_instance()) {
        DPRINTF(HMVController, "Begin instance called\n");
        allow_error_injection = true;
        instance_start_time = curCycle();
    } else {
        DPRINTF(HMVController, "Begin warmup instance\n");
    }
    DPRINTF(HMVController, "Instance start time: %d\n", instance_start_time);
}

bool HMVController::begin_thread(ThreadID tid) {
    if (core->begin_thread(tid)) {
        DPRINTF(HMVController, "Begin thread called @%02d\n", tid);
        return true;
    }
    return false;
}

bool HMVController::end_thread(ThreadID tid) {
    if (core->end_thread(tid, end_simulation)) {
        DPRINTF(HMVController, "End thread called @%02d\n", tid);
        return true;
    }
    return false;
}

void HMVController::end_instance() {
    if (core->end_instance(end_simulation)) {
        DPRINTF(HMVController, "End instance called\n");
        allow_error_injection = false;
    } else {
        DPRINTF(HMVController, "End warmup instance\n");
    }
}

void HMVController::end_test() {
    DPRINTF(HMVController, "End test called\n");
    core->end_test(end_simulation);
}

void HMVController::end_program() {
    DPRINTF(HMVController, "End program called\n");
}

template <>
void HMVController::push_event(const WriteAt<Monitor::AVAILABILITY>& event) {
    core->push_event(event, end_simulation);
}

template <>
void HMVController::push_event(const ReadAt<Monitor::LOCAL_COMMIT>& event) {
    core->push_event(event, end_simulation);
}

template <>
void HMVController::push_event(const WriteAt<Monitor::LOCAL_COMMIT>& event) {
    core->push_event(event, end_simulation);
}

template <>
void HMVController::push_event(const FenceAt<Monitor::LOCAL_COMMIT>& event) {
    core->push_event(event, end_simulation);
}

template <>
void HMVController::push_event(const ReadAt<Monitor::LOCAL_COMPLETION>& event) {
    core->push_event(event, end_simulation);
}

template <>
void HMVController::push_event(
    const WriteAt<Monitor::LOCAL_COMPLETION>& event) {
    core->push_event(event, end_simulation);
}

template <>
void HMVController::push_event(const ReadAt<Monitor::MEMORY>& event) {
    core->push_event(event, end_simulation);
}

template <>
void HMVController::push_event(const WriteAt<Monitor::MEMORY>& event) {
    core->push_event(event, end_simulation);
}

template <>
void HMVController::push_event(const ReadAt<Monitor::ENTER_TIME>& event) {
    core->push_event(event, end_simulation);
}

template <>
void HMVController::push_event(const WriteAt<Monitor::ENTER_TIME>& event) {
    core->push_event(event, end_simulation);
}

template <>
void HMVController::push_event(const FenceAt<Monitor::ENTER_TIME>& event) {
    core->push_event(event, end_simulation);
}

template <>
void HMVController::push_event(const ReadAt<Monitor::COMMIT_TIME>& event) {
    core->push_event(event, end_simulation);
}

template <>
void HMVController::push_event(const WriteAt<Monitor::COMMIT_TIME>& event) {
    core->push_event(event, end_simulation);
}

template <>
void HMVController::push_event(const FenceAt<Monitor::COMMIT_TIME>& event) {
    core->push_event(event, end_simulation);
}

template <>
void HMVController::push_event(const WriteAt<Monitor::REMOTE_COMMIT>& event) {
    core->push_event(event, end_simulation);
}

template <>
void HMVController::push_event(
    const WriteAt<Monitor::REMOTE_COMPLETION>& event) {
    core->push_event(event, end_simulation);
}

template <>
void HMVController::push_event(const ReadAt<Monitor::SQUASH>& event) {
    core->push_event(event, end_simulation);
}

template <>
void HMVController::push_event(const WriteAt<Monitor::SQUASH>& event) {
    core->push_event(event, end_simulation);
}

void HMVController::push_transition(const std::string& fsm,
                                    const std::string& state,
                                    const std::string& event,
                                    const std::string& next_state,
                                    Addr line_address,
                                    unsigned instance_id) {
    core->push_transition(
        fsm, state, event, next_state, line_address, instance_id);
}

bool HMVController::thread_barrier_wait(ThreadID tid, ThreadContext* tc) {
    return thread_barrier.wait(tid, tc);
}

std::optional<Addr> HMVController::match_address(Addr paddr) const {
    if (paddr >= shared_physical_region) {
        auto address = paddr - shared_physical_region;
        if (core->match_address(address)) {
            return address;
        }
    }
    return {};
}

uint64_t HMVController::get_value(const void* bytes) const {
    return get_value(bytes, guest_word_size);
}

uint64_t HMVController::get_value(const void* bytes, size_t size) const {
    auto [value, success] = getUintX(bytes, size, guest_byte_order);
    assert(success);
    return value;
}

uint64_t HMVController::load_test(ThreadContext* tc,
                                  uint64_t size,
                                  Addr vaddr) {
    if (generator_pid >= 0 || is_first_test()) {
        if (hmv::platform::signal_continue_and_wait(generator_pid, 0) > 0) {
            auto instances = core->load_test();
            if (instances > 0) {
                assert(size == code_memory_size);
                auto proxy = make_proxy(tc);
                auto max_thread_size = core->max_encoded_thread_size();
                auto current_offset = vaddr;
                for (auto& thread : core->encoded_threads()) {
                    assert(thread.size() <= max_thread_size);
                    proxy->writeBlob(
                        current_offset, thread.data(), thread.size());
                    current_offset += max_thread_size;
                }
            }
            return instances;
        }
    }
    return 0;
}

uint64_t HMVController::load_addresses(ThreadContext* tc,
                                       uint64_t size,
                                       Addr vaddr) {
    constexpr auto address_size = sizeof(hmv::Address);
    auto addresses = core->encoded_addresses();
    auto vector_size = addresses.size();
    assert(vector_size <= (size * sizeof(hmv::Address)));
    for (auto i = 0u; i < addresses.size(); i += address_size) {
        validate_address(get_value(&addresses[i], address_size));
    }
    auto proxy = make_proxy(tc);
    proxy->writeBlob(vaddr, addresses.data(), vector_size);
    return vector_size / address_size;
}

void HMVController::destroy() {
    if (core->quiesce()) {
        hmv::platform::signal_continue(generator_pid, 0);
    }
    core.reset();
}

bool HMVController::is_first_test() const {
    return instance_start_time == 0;
}

void HMVController::validate_address(Addr addr) {
    DPRINTF(HMVController,
            "Shared address %#x (P:%#x = V:%#x)\n",
            addr,
            shared_physical_region + addr,
            shared_memory_region + addr);
    assert(core->match_address(addr));
}

HMVController::ThreadBarrier::ThreadBarrier(uint16_t thread_count)
    : generation{0}, local_generations(thread_count, 0) {}

bool HMVController::ThreadBarrier::wait(uint16_t tid, ThreadContext* tc) {
    if (suspended_threads.count(tc)) {
        tc->suspend();
        return true;
    }
    if (local_generations[tid] != generation) {
        local_generations[tid] = generation;
        return false;
    }
    if (suspended_threads.size() == local_generations.size() - 1) {
        for (auto other : suspended_threads) {
            other->activate();
        }
        suspended_threads.clear();
        ++generation;
        return true;
    }
    suspended_threads.insert(tc);
    tc->suspend();
    return true;
}
