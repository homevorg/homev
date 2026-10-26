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

#include "hmv_thread_monitor.hh"

#include <limits>

#include "hmv/core/event.hpp"
#include "hmv/core/types.hpp"

#include "cpu/o3/dyn_inst.hh"
#include "debug/HMVThreadMonitor.hh"
#include "hmv_controller.hh"
#include "mem/ruby/common/DataBlock.hh"

using gem5::HMVThreadMonitor;
using Monitor = hmv::Monitor;
template <Monitor source>
using FenceAt = hmv::FenceAt<source>;
template <Monitor source>
using ReadAt = hmv::ReadAt<source>;
template <Monitor source>
using WriteAt = hmv::WriteAt<source>;

HMVThreadMonitor::HMVThreadMonitor(const Params& p)
    : SimObject(p), controller{*p.controller},
      tid{static_cast<uint16_t>(p.tid)} {}

void HMVThreadMonitor::begin_thread(Addr pc) {
    if (controller.begin_thread(tid) && !started) {
        DPRINTF(HMVThreadMonitor, "Monitor enabled (PC: 0x%016x)\n", pc);
        started = true;
        initial_pc = pc;
    }
}

void HMVThreadMonitor::end_thread() {
    if (controller.end_thread(tid) && started) {
        DPRINTF(HMVThreadMonitor,
                "Monitor disabled (after %d operations)\n",
                operation_count);
        started = false;
        operation_count = 0;
    }
}

void HMVThreadMonitor::availability(const o3::DynInstPtr& inst,
                                    uint8_t* data,
                                    unsigned size) {
    if (!started) {
        return;
    }
    if (auto address = controller.match_address(inst->physEffAddr)) {
        auto value = controller.get_value(data, size);
        // Save the stored data to later be used in the commit event
        inst->hmvAvailabilityData = value;
        auto pc = normalize_pc(inst);
        auto time = controller.current_time();
        controller.push_event(WriteAt<Monitor::AVAILABILITY>{
            address.value(), value, tid, pc, time});
    }
}

void HMVThreadMonitor::memory_read(const o3::DynInstPtr& inst) {
    if (!started) {
        return;
    }
    if (auto address = controller.match_address(inst->physEffAddr)) {
        auto pc = normalize_pc(inst);
        auto time = controller.current_time();
        controller.push_event(ReadAt<Monitor::MEMORY>{
            address.value(), get_value(inst), tid, pc, time});
    }
}

void HMVThreadMonitor::local_commit(const o3::DynInstPtr& inst) {
    if (!started) {
        return;
    }
    auto time = controller.current_time();
    if (inst->isFullMemBarrier() && is_inside_test(inst)) {
        ++operation_count;
        auto pc = normalize_pc(inst);
        controller.push_event(FenceAt<Monitor::LOCAL_COMMIT>{tid, pc, time});
        controller.push_event(FenceAt<Monitor::COMMIT_TIME>{tid, pc, time});
    } else if (auto address = controller.match_address(inst->physEffAddr)) {
        auto pc = normalize_pc(inst);
        if (inst->isLoad()) {
            ++operation_count;
            auto value = get_value(inst);
            controller.push_event(ReadAt<Monitor::LOCAL_COMMIT>{
                address.value(), value, tid, pc, time});
            controller.push_event(ReadAt<Monitor::COMMIT_TIME>{
                address.value(), value, tid, pc, time});
        } else if (inst->isStore()) {
            ++operation_count;
            controller.push_event(WriteAt<Monitor::LOCAL_COMMIT>{
                address.value(), inst->hmvAvailabilityData, tid, pc, time});
        }
    }
}

void HMVThreadMonitor::local_completion_read(const o3::DynInstPtr& inst) {
    if (!started) {
        return;
    }
    if (auto address = controller.match_address(inst->physEffAddr)) {
        auto pc = normalize_pc(inst);
        auto time = controller.current_time();
        controller.push_event(ReadAt<Monitor::LOCAL_COMPLETION>{
            address.value(), get_value(inst), tid, pc, time});
    }
}

void HMVThreadMonitor::local_completion_write(const o3::DynInstPtr& inst) {
    if (!started) {
        return;
    }
    if (auto address = controller.match_address(inst->physEffAddr)) {
        auto time = controller.current_time();
        auto pc = normalize_pc(inst);
        auto value = get_value(inst);
        controller.push_event(WriteAt<Monitor::LOCAL_COMPLETION>{
            address.value(), value, tid, pc, time});
        controller.push_event(
            WriteAt<Monitor::MEMORY>{address.value(), value, tid, pc, time});
        controller.push_event(WriteAt<Monitor::COMMIT_TIME>{
            address.value(), value, tid, pc, time});
    }
}

void HMVThreadMonitor::remote_commit(Addr paddr, const ruby::DataBlock& block) {
    if (!started) {
        return;
    }
    auto time = controller.current_time();
    auto block_size = block.getBlockSize();
    auto data = block.getData(0, block_size);
    auto word_size = controller.word_size();
    for (unsigned byte = 0; byte < block_size; byte += word_size) {
        if (auto address = controller.match_address(paddr + byte)) {
            controller.push_event(WriteAt<Monitor::REMOTE_COMMIT>{
                address.value(),
                controller.get_value(&data[byte]),
                tid,
                0,
                time});
        }
    }
}

void HMVThreadMonitor::remote_completion(Addr paddr,
                                         const ruby::DataBlock& block) {
    if (!started) {
        return;
    }
    auto time = controller.current_time();
    auto block_size = block.getBlockSize();
    auto data = block.getData(0, block_size);
    auto word_size = controller.word_size();
    for (unsigned byte = 0; byte < block_size; byte += word_size) {
        if (auto address = controller.match_address(paddr + byte)) {
            controller.push_event(WriteAt<Monitor::REMOTE_COMPLETION>{
                address.value(),
                controller.get_value(&data[byte]),
                tid,
                0,
                time});
        }
    }
}

void HMVThreadMonitor::squash_read(const o3::DynInstPtr& inst) {
    if (!started || !inst->isExecuted()) {
        return;
    }
    if (auto address = controller.match_address(inst->physEffAddr)) {
        auto pc = normalize_pc(inst);
        auto time = controller.current_time();
        controller.push_event(ReadAt<Monitor::SQUASH>{
            address.value(), get_value(inst), tid, pc, time});
    }
}

void HMVThreadMonitor::squash_write(const o3::DynInstPtr& inst) {
    if (!started || !inst->isExecuted()) {
        return;
    }
    if (auto address = controller.match_address(inst->physEffAddr)) {
        auto pc = normalize_pc(inst);
        auto time = controller.current_time();
        controller.push_event(WriteAt<Monitor::SQUASH>{
            address.value(), inst->hmvAvailabilityData, tid, pc, time});
    }
}

void HMVThreadMonitor::enter_time(const o3::DynInstPtr& inst) {
    if (!started || !is_inside_test(inst)) {
        return;
    }
    auto pc = normalize_pc(inst);
    auto time = controller.current_time();
    if (inst->isFullMemBarrier()) {
        controller.push_event(FenceAt<Monitor::ENTER_TIME>{tid, pc, time});
    } else if (inst->isLoad()) {
        auto address = controller.match_address(inst->physEffAddr);
        controller.push_event(
            ReadAt<Monitor::ENTER_TIME>{address.value_or(0), 0, tid, pc, time});
    } else if (inst->isStore()) {
        auto address = controller.match_address(inst->physEffAddr);
        controller.push_event(WriteAt<Monitor::ENTER_TIME>{
            address.value_or(0), 0, tid, pc, time});
    }
}

bool HMVThreadMonitor::thread_barrier_wait(ThreadContext* tc) {
    return controller.thread_barrier_wait(tid, tc);
}

uint64_t HMVThreadMonitor::get_value(const gem5::o3::DynInstPtr& inst) {
    return controller.get_value(inst->memData, inst->effSize);
}

bool HMVThreadMonitor::is_inside_test(const o3::DynInstPtr& inst) const {
    return inst->pcState().instAddr() >= initial_pc;
}

unsigned HMVThreadMonitor::normalize_pc(const o3::DynInstPtr& inst) const {
    auto normalized_pc = inst->pcState().instAddr() - initial_pc;
    assert(normalized_pc <= std::numeric_limits<hmv::ProgramCounter>::max());
    return normalized_pc;
}
