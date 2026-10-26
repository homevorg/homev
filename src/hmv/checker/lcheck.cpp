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

#include "lcheck.hpp"
#include "hmv/core/types.hpp"

#include <algorithm>
#include <cassert>
#include <deque>

using hmv::checker::LCHECK;
using hmv::checker::Report;

namespace hmv::checker {
    template <>
    struct ErrorInfo<LCHECK::Error::NONE> {
        static constexpr auto description = "no error found";
    };

    template <>
    struct ErrorInfo<LCHECK::Error::PO_CYCLE> {
        static constexpr auto description =
            "cycle found when adding processor order edge";
    };

    template <>
    struct ErrorInfo<LCHECK::Error::CYCLE_FOUND> {
        static constexpr auto description = "cycle found during checking";
    };

    template <>
    struct ErrorInfo<LCHECK::Error::INVALID_SERIALIZATION> {
        static constexpr auto description = "invalid serialization of writes";
    };

    template <>
    struct ErrorInfo<LCHECK::Error::INVALID_READ_MAPPING> {
        static constexpr auto description =
            "invalid mapping between reads and writes";
    };

    template <>
    struct ErrorInfo<LCHECK::Error::EXEC_CYCLE> {
        static constexpr auto description =
            "cycle found when adding execution order edge";
    };

    template <>
    struct ErrorInfo<LCHECK::Error::INFERRED_CYCLE> {
        static constexpr auto description =
            "cycle found when adding inferred edge";
    };
} // namespace hmv::checker

LCHECK::LCHECK(ThreadCount thread_count, ISATraits traits)
    : tracker{thread_count} {
    assert(traits.memory_model == MCM::RELAXED);
    assert(traits.atomicity == Atomicity::OMCA);
}

void LCHECK::begin_test(const hmv::Test& test) {
    error_report = {};
}

bool LCHECK::end_instance() {
    tests.push_back(tracker.extract_test());
    tracker.reset();
    return false;
}

bool LCHECK::end_test() {
    for (auto& test : tests) {
        test.build_graph();
        if (test.compute_read_mapping()) {
            error_report = make_report<Error::INVALID_READ_MAPPING>();
        } else if (test.add_processor_order()) {
            error_report = make_report<Error::PO_CYCLE>();
        } else if (test.add_execution_order()) {
            error_report = make_report<Error::EXEC_CYCLE>();
        } else if (test.infer_edges()) {
            error_report = make_report<Error::INFERRED_CYCLE>();
        } else {
            error_report = test.check_cycles();
        }
        if (error_report.code != Error::NONE) {
            return true;
        }
    }
    tests.clear();
    return false;
}

LCHECK::Test::Test(const std::vector<Thread>& threads) : threads{threads} {}

bool LCHECK::Test::add_execution_order() {
    for (auto& thread : threads) {
        auto last_write = std::unordered_map<Address, Node*>{};
        for (auto& v : thread.nodes) {
            if (v.event.is_load()) {
                auto w = producers[v.event.value];
                if (time_order(&v, w)) {
                    return true;
                }
                // execution order edge
                add_edge(w, &v);
                if (auto w_prime = last_write[v.event.address]) {
                    if (w != w_prime) {
                        if (time_order(w, w_prime)) {
                            return true;
                        }
                        // observed edge
                        add_edge(w_prime, w);
                    }
                }
            } else if (v.event.is_store()) {
                last_write[v.event.address] = &v;
            }
        }
    }
    return false;
}

bool LCHECK::Test::add_processor_order() {
    for (auto& thread : threads) {
        auto last = std::unordered_map<Address, Node*>{};
        for (auto& v : thread.nodes) {
            if (v.event.is_fence()) {
                for (auto address : thread.addresses) {
                    if (auto u = last[address]) {
                        if (time_order(&v, u)) {
                            return true;
                        }
                        add_edge(u, &v);
                    }
                    last[address] = &v;
                }
                continue;
            }
            if (auto u = last[v.event.address]) {
                if (time_order(&v, u)) {
                    return true;
                }
                add_edge(u, &v);
            }
            last[v.event.address] = &v;
        }
    }
    return false;
}

void LCHECK::Test::build_graph() {
    auto graph_size = uint64_t{1};
    for (auto& thread : threads) {
        graph_size += thread.nodes.size();
    }
    nodes.reserve(graph_size);
    nodes.push_back(&root_node);
    for (auto& thread : threads) {
        for (auto& node : thread.nodes) {
            nodes.push_back(&node);
        }
    }
    compute_pending_periods();
}

Report LCHECK::Test::check_cycles() {
    auto latest = std::unordered_map<Address, std::deque<Test::Node*>>{};
    auto search = [this](std::deque<Test::Node*>& nodes, Node* u) -> Node* {
        for (auto candidate : nodes) {
            if (time_order(candidate, u)) {
                return candidate;
            }
        }
        return nullptr;
    };
    for (auto u : nodes) {
        if (u->event.is_load()) {
            if (auto w = search(latest[u->event.address], u)) {
                auto w_prime = producers[u->event.value];
                if (time_order(w_prime, w) || global_order(w_prime, w)) {
                    return make_report<Error::INVALID_SERIALIZATION>();
                }
            }
        } else if (u->event.is_store()) {
            latest[u->event.address].push_front(u);
        }
        if (global_order(u, u)) {
            return make_report<Error::CYCLE_FOUND>();
        }
        for (auto [v, _] : u->forward_closure) {
            if (time_order(v, u)) {
                return make_report<Error::CYCLE_FOUND>();
            }
        }
    }
    return {};
}

bool LCHECK::Test::compute_read_mapping() {
    producers[0] = {&root_node};
    for (auto& thread : threads) {
        for (auto& node : thread.nodes) {
            if (node.event.is_store()) {
                if (producers[node.event.value]) {
                    return true;
                }
                producers[node.event.value] = &node;
            } else if (node.event.is_load()) {
                consumers[node.event.value].push_back(&node);
            }
        }
    }
    for (auto& [value, reads] : consumers) {
        if (!reads.empty() && !producers[value]) {
            return true;
        }
    }
    return false;
}

bool LCHECK::Test::infer_edges() {
    for (auto node : nodes) {
        if (!node->successors.empty() && infer_edge(node)) {
            return true;
        }
    }
    return false;
}

bool LCHECK::Test::add_closure(Node* u, Node* v) {
    if (overlap(u, v) && u->forward_closure.insert({v, false}).second) {
        v->backward_closure.insert(u);
        return true;
    }
    return false;
}

void LCHECK::Test::add_collision(Node* u, Node* v) {
    if (v->event.is_load()) {
        u->overlapped_colliding_reads.push_back(v);
    } else if (v->event.is_store()) {
        v->overlapped_colliding_writes.push_back(v);
    }
}

bool LCHECK::Test::add_edge(Node* u, Node* v) {
    if (overlap(u, v) && u->successors.insert(v).second) {
        if (add_closure(u, v)) {
            for (auto [f, _] : v->forward_closure) {
                add_closure(u, f);
            }
            for (auto b : u->backward_closure) {
                if (add_closure(b, v)) {
                    for (auto [f, _] : v->forward_closure) {
                        add_closure(b, f);
                    }
                }
            }
        }
        return true;
    }
    return false;
}

void LCHECK::Test::add_overlapped(Node* u, Node* v) {
    if (v->event.is_store()) {
        u->overlapped_writes.push_back(v);
    }
}

void LCHECK::Test::compute_pending_periods() {
    std::sort(nodes.begin(), nodes.end(), [](auto i, auto j) {
        return i->period.begin < j->period.begin
               || (i->period.begin == j->period.begin
                   && i->period.end < j->period.end);
    });
    auto lower_bound = 0u;
    for (auto current = 0u; current < nodes.size(); ++current) {
        auto cur_node = nodes[current];
        while (time_order(nodes[lower_bound], cur_node)) {
            ++lower_bound;
        }
        add_overlapped(cur_node, cur_node);
        for (auto previous = lower_bound; previous < current; ++previous) {
            auto prev_node = nodes[previous];
            if (prev_node->period.end >= cur_node->period.begin) {
                add_overlapped(prev_node, cur_node);
                add_overlapped(cur_node, prev_node);
                if (cur_node->event.address == prev_node->event.address) {
                    add_collision(cur_node, prev_node);
                    add_collision(prev_node, cur_node);
                }
            }
        }
    }
}

bool LCHECK::Test::global_order(Node* u, Node* v) {
    return u->forward_closure.count(v);
}

bool LCHECK::Test::infer_edge(Node* u) {
    for (auto w : u->overlapped_writes) {
        for (auto r : w->overlapped_colliding_reads) {
            auto w_prime = producers[r->event.value];
            if (w_prime == w || time_order(w, w_prime)) {
                continue;
            }
            if (global_order(w, r) && !is_inferred(w, r)) {
                if (time_order(w_prime, w)) {
                    return true;
                }
                if (add_edge(w, w_prime) && infer_edge(w)) {
                    return true;
                }
                set_inferred(w, r);
            }
        }
        for (auto w_prime : w->overlapped_colliding_writes) {
            if (global_order(w, w_prime) && !is_inferred(w, w_prime)) {
                for (auto r : consumers[w->event.value]) {
                    if (time_order(w_prime, r)) {
                        return true;
                    }
                    if (time_order(r, w_prime)) {
                        continue;
                    }
                    if (add_edge(r, w_prime) && infer_edge(r)) {
                        return true;
                    }
                }
                set_inferred(w, w_prime);
            }
        }
    }
    return false;
}

bool LCHECK::Test::is_inferred(Node* u, Node* v) {
    return u->forward_closure.at(v);
}

void LCHECK::Test::set_inferred(Node* u, Node* v) {
    u->forward_closure.at(v) = true;
}

bool LCHECK::Test::overlap(Node* u, Node* v) {
    return u->period.end >= v->period.begin && v->period.end >= u->period.begin;
}

bool LCHECK::Test::time_order(Node* u, Node* v) {
    return u->period.end < v->period.begin;
}

LCHECK::Tracker::Tracker(ThreadCount thread_count)
    : threads{thread_count}, tracks{thread_count} {}

void LCHECK::Tracker::begin(const Event& event) {
    tracks[event.tid][event.pc].enter_time = event.time;
}

void LCHECK::Tracker::end(const Event& event) {
    auto& track = tracks[event.tid][event.pc];
    auto& node = threads[event.tid].nodes[track.index];
    node.period.begin = track.enter_time;
    node.period.end = event.time;
}

LCHECK::Test LCHECK::Tracker::extract_test() const {
    return {threads};
}

void LCHECK::Tracker::program_order(const Event& event) {
    assert(tracks[event.tid].count(event.pc));
    tracks[event.tid][event.pc].index = threads[event.tid].nodes.size();
    threads[event.tid].nodes.push_back({event});
    if (!event.is_fence()) {
        threads[event.tid].addresses.insert(event.address);
    }
}

void LCHECK::Tracker::reset() {
    for (auto& thread : threads) {
        thread.nodes.clear();
        thread.addresses.clear();
    }
    for (auto& track : tracks) {
        track.clear();
    }
}
