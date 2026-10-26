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

#ifndef __HMV_CHECKER_LCHECK_HPP__
#define __HMV_CHECKER_LCHECK_HPP__

#include <set>
#include <unordered_map>
#include <unordered_set>

#include "component/report.hpp"
#include "hmv/core/event.hpp"
#include "hmv/core/test.hpp"
#include "hmv/core/types.hpp"

namespace hmv::checker {
    class LCHECK {
     public:
        enum Error : ErrorCode {
            NONE = 0,
            PO_CYCLE = 1,
            CYCLE_FOUND = 2,
            INVALID_SERIALIZATION = 3,
            INVALID_READ_MAPPING = 4,
            EXEC_CYCLE = 5,
            INFERRED_CYCLE = 6,
        };

        LCHECK(ThreadCount thread_count, ISATraits traits = {});
        void begin_test(const Test& test);
        bool end_instance();
        bool end_test();
        template <template <Monitor> typename EventAt>
        bool push(const EventAt<LOCAL_COMMIT>& event);
        template <template <Monitor> typename EventAt>
        bool push(const EventAt<ENTER_TIME>& event);
        template <template <Monitor> typename EventAt>
        bool push(const EventAt<COMMIT_TIME>& event);
        const Report& report() const;

     private:
        struct Test {
            struct Node {
                struct PendingPeriod {
                    Time begin{};
                    Time end{};
                };

                Event event{};
                PendingPeriod period{};
                std::unordered_set<Node*> successors{};
                std::unordered_set<Node*> backward_closure{};
                std::unordered_map<Node*, bool> forward_closure{};
                std::vector<Node*> overlapped_colliding_reads{};
                std::vector<Node*> overlapped_colliding_writes{};
                std::vector<Node*> overlapped_writes{};
            };

            struct Thread {
                std::vector<Node> nodes{};
                std::set<Address> addresses{};
            };

            Test(const std::vector<Thread>& threads);

            bool add_execution_order();
            bool add_processor_order();
            void build_graph();
            Report check_cycles();
            bool compute_read_mapping();
            bool infer_edges();

         private:
            Node root_node{};
            std::vector<Thread> threads{};
            std::vector<Node*> nodes{};
            std::unordered_map<hmv::Data, std::vector<Node*>> consumers{};
            std::unordered_map<hmv::Data, Node*> producers{};

            bool add_closure(Node* u, Node* v);
            void add_collision(Node* u, Node* v);
            bool add_edge(Node* u, Node* v);
            void add_overlapped(Node* u, Node* v);
            void compute_pending_periods();
            bool global_order(Node* u, Node* v);
            bool infer_edge(Node* u);
            bool is_inferred(Node* u, Node* v);
            bool overlap(Node* u, Node* v);
            void set_inferred(Node* u, Node* v);
            bool time_order(Node* u, Node* v);
        };

        class Tracker {
            struct Track {
                uint64_t enter_time{};
                size_t index{};
            };

         public:
            Tracker(ThreadCount thread_count);
            void begin(const Event& event);
            void end(const Event& event);
            Test extract_test() const;
            void program_order(const Event& event);
            void reset();

         private:
            std::vector<Test::Thread> threads{};
            std::vector<std::unordered_map<uint64_t, Track>> tracks;
        };

        Tracker tracker;
        std::vector<Test> tests{};
        Report error_report{};
    };

    template <template <Monitor> typename EventAt>
    inline bool LCHECK::push(const EventAt<ENTER_TIME>& event) {
        tracker.begin(event);
        return false;
    }
    template <template <Monitor> typename EventAt>
    inline bool LCHECK::push(const EventAt<COMMIT_TIME>& event) {
        tracker.end(event);
        return false;
    }
    template <template <Monitor> typename EventAt>
    inline bool LCHECK::push(const EventAt<LOCAL_COMMIT>& event) {
        tracker.program_order(event);
        return false;
    }

    inline const Report& LCHECK::report() const {
        return error_report;
    }
} // namespace hmv::checker

#endif // __HMV_CHECKER_LCHECK_HPP__
