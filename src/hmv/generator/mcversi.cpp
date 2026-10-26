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

#include "mcversi.hpp"

#include <unordered_map>

#include "component/mcversi/test.hpp"
#include "hmv/core/operation.hpp"
#include "hmv/core/register.hpp"
#include "hmv/core/test.hpp"
#include "hmv/core/trace.hpp"
#include "hmv/core/types.hpp"

using hmv::Test;
using hmv::generator::McVerSi;

McVerSi::McVerSi(ThreadCount thread_count,
                 ByteCount memory_size,
                 ByteCount stride,
                 InstanceCount instances,
                 TestSize test_size,
                 size_t population_size,
                 size_t tournament_size,
                 size_t mates,
                 float mutation_rate,
                 double p_usel,
                 double p_bfa,
                 ByteCount chunk_size,
                 ByteCount hole_size,
                 TestSize nop_max_seq_size,
                 const mcversi::OperationMix& mix,
                 Seed seed)
    : thread_count{thread_count}, instances{instances},
      tournament_size{std::min(tournament_size, population_size)},
      mates{std::min(mates, population_size)}, pool{thread_count,
                                                    memory_size,
                                                    stride,
                                                    test_size,
                                                    population_size,
                                                    mutation_rate,
                                                    chunk_size,
                                                    hole_size,
                                                    nop_max_seq_size,
                                                    mix,
                                                    seed},
      crossover_mutate{p_usel, p_bfa},
      selection{pool.select(mates * tournament_size)},
      register_allocator{seed} {}

std::optional<hmv::Test> McVerSi::operator()() {
    while (selection[test_index]->fitness > 0
           || !selection[test_index]->fitaddrs.empty()) {
        if (++test_index == mates * tournament_size) {
            selection = pool.tournament(
                selection, tournament_size, mates, crossover_mutate);
            test_index = 0;
            ++current_tournament;
        }
    }
    auto test = extract_test(*selection[test_index]);
    address_count = test.addresses.size();
    return test;
}

void McVerSi::feedback(const platform::Output& output) {
    fitaddrs_collector.hard_reset();
    for (auto& instance : output.trace.instances) {
        for (auto event : instance) {
            fitaddrs_collector.add(event);
        }
        fitaddrs_collector.soft_reset();
    }
    selection[test_index]->fitness = output.metrics.front().value();
    selection[test_index]->fitaddrs = fitaddrs_collector.fitaddrs();
    selection[test_index]->nd_t = fitaddrs_collector.nd_t();
}

Test McVerSi::extract_test(const mcversi::Test& mcversi_test) {
    auto value = Data{1};
    auto seen = std::unordered_map<Address, AddressCount>{};
    auto addresses = std::vector<Address>{};
    auto threads = std::vector<Thread>{thread_count};
    auto registers = std::vector<std::set<Register>>{thread_count};
    for (auto& pop : mcversi_test.operations) {
        if (!seen.count(pop.address)) {
            seen[pop.address] = addresses.size();
            addresses.push_back(pop.address);
        }
        switch (pop.type) {
        case mcversi::ProtoOperation::READ_ADDR_DEP:
            if (!registers[pop.tid].empty()) {
                auto op = register_allocator.randomize(
                    Load{seen[pop.address],
                         Register::NONE,
                         Register::NONE,
                         {register_allocator.random(registers[pop.tid])}});
                registers[pop.tid].insert(op.dest);
                threads[pop.tid].push_back(Xor{op.src2, op.src2, op.src2});
                threads[pop.tid].push_back(
                    Add{op.src1, {op.src1, Register::ASSIGN_ADDRESS}, op.src2});
                op.src1.usage(Register::PRESERVE);
                threads[pop.tid].push_back(op);
                break;
            }
            [[fallthrough]];
        case mcversi::ProtoOperation::READ: {
            auto op = register_allocator.randomize(Load{seen[pop.address]});
            registers[pop.tid].insert(op.dest);
            threads[pop.tid].push_back(op);
            break;
        }
        case mcversi::ProtoOperation::WRITE:
            threads[pop.tid].push_back(register_allocator.randomize(
                Store{seen[pop.address], value++}));
            break;
        case mcversi::ProtoOperation::FENCE:
            threads[pop.tid].push_back(Fence{});
            break;
        case mcversi::ProtoOperation::DELAY:
            threads[pop.tid].push_back(Nop{pop.address});
            break;
        default:
            assert(false);
            break;
        }
    }
    return {addresses, threads, instances};
}
