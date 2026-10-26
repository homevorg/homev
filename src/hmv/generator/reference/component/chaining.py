# Copyright (C) 2015-2025 Gabriel Arthur Gerber Andrade

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

""" Instruction Generation Module """

import random as prng

from .biasing import Generator as AddressGenerator
from .common import Hash, BaseGenerator
from hmv import Load, Fence, Store, Test


class Generator(BaseGenerator):
    def generate_program(
        self,
        seed,
        cores,
        operations_per_core,
        shared_addresses,
        categories_mix,
        instances,
        variable_thread_length=False,
        **kwargs,
    ):
        clique_number = kwargs["clique_number"]
        igm_hash = [seed, cores, operations_per_core, shared_addresses]
        igm_hash.extend(categories_mix)
        addresses = AddressGenerator(
            seed=Hash(seed, cores, operations_per_core), **kwargs
        ).select(shared_addresses)
        self.ng = prng.Random(Hash(*igm_hash))

        class Global:
            def __init__(self):
                self.n = operations_per_core * cores
                self.p = cores
                self.s = shared_addresses
                self.k = clique_number
                self.threads = []
                self.cores = range(cores)
                self.shared_addresses = range(shared_addresses)
                self.operations = [Load, Store, Fence]
                self.load_store_mix = [0.5, 0.5]
                self.max_min_length = 4
                self.last = {}
                self.all_slots = self.n
                self.free_slots = {}
                self.candidates_by_free_slots = [
                    [] for _ in range(self.max_min_length + 1)
                ]
                self.category_free_slots = [
                    round(self.n * categories_mix[idx]) for idx in range(4)
                ]
                self.extension_min_cost = 3
                self.id_chain = None
                self.limited_by_mix = True
                for i in self.cores:
                    self.threads.append([])
                    if variable_thread_length:
                        self.free_slots[i] = int(self.n)
                    else:
                        self.free_slots[i] = int(operations_per_core)
                    self.candidates_by_free_slots[self.cropped_slots_of(i)].append(i)
                    for a in self.shared_addresses:
                        self.last[i, a] = None
                # Constraint about minimum addressses
                self.category_min_s = [1, 1, 2, 2]
                self.category_min_n = [1, 3, 6, 7]
                self.remain_n = [
                    self.category_min_s[idx]
                    * int(self.category_free_slots[idx] / self.category_min_n[idx])
                    for idx in range(4)
                ]
                self.remain_s = list(self.shared_addresses)
                self.store_value = 1

            def cropped_slots_of(self, i):
                return max(min(self.free_slots[i], self.max_min_length), 0)

        # Variables shared among classes (ugly)
        scope = Global()
        # Keep pointer to be able to call method @choice
        ng = self

        def demote(i, by):
            # Stop condition
            scope.all_slots -= by
            # Constraint about operations per core
            if variable_thread_length:
                demote_cores = range(scope.p)
            else:
                demote_cores = [i]
            assert scope.p == len(scope.free_slots)
            for j in demote_cores:
                before = scope.cropped_slots_of(j)
                scope.free_slots[j] -= by
                after = scope.cropped_slots_of(j)
                if before != after:
                    scope.candidates_by_free_slots[before].remove(j)
                    scope.candidates_by_free_slots[after].append(j)
            # Constraint about operations per category
            scope.category_free_slots[scope.id_chain] -= by

        def add_fence(i):
            scope.threads[i].append(Fence())

        def add_operation(Type, i, a, spo=False):
            if spo:
                add_fence(i)
            if a in scope.remain_s:
                scope.remain_s.remove(a)
            if Type == Store:
                scope.threads[i].append(Type(a, scope.store_value))
                scope.store_value += 1
            else:
                scope.threads[i].append(Type(a))
            scope.last[i, a] = Type

        def Any():
            return ng.choice(scope.operations[:2], scope.load_store_mix)

        def simulate_add_sco(i, j, spo=False):
            remotes = sum(scope.candidates_by_free_slots[1:], [])
            candidates = []
            if (
                scope.category_free_slots[scope.id_chain] > 1
                or not scope.limited_by_mix
            ):
                if scope.free_slots[i] >= 1:
                    remotes.remove(i)
                    candidates.append(i)
                if scope.free_slots[j] >= 1:
                    remotes.remove(j)
                    candidates.append(j)
                if len(remotes) != 0:
                    candidates.append("remote")
            op1 = Any()
            op2 = Store if (op1 == Load and len(candidates) == 0) else Any()
            block = []
            block.append([op1, i, spo])
            if op1 == Load and op2 == Load:
                k = ng.choice(candidates)
                if k == "remote":
                    k = ng.choice(remotes)
                demote(k, 1)
                block.append([Store, k])
            block.append([op2, j])
            return block

        def att_addr(a, block):
            for inst_args in block:
                inst_args.insert(2, a)
                add_operation(*inst_args)

        def add_sco(i, j, a, spo=False):
            block = simulate_add_sco(i, j, spo)
            att_addr(a, block)

        def remove_if_exists(src, item):
            try:
                src.remove(item)
            except:
                pass

        def choose_address(from_addresses, demoted=False):
            min_length = scope.category_min_n[scope.id_chain]
            if scope.limited_by_mix:
                if sum(scope.remain_n) > len(scope.remain_s):
                    a = ng.choice(from_addresses)
                else:
                    a = ng.choice(scope.remain_s)
                scope.remain_n[scope.id_chain] -= 1
                if a in scope.remain_s:
                    scope.remain_s.remove(a)
                available_globaly = sum(scope.remain_n) - scope.remain_n[scope.id_chain]
                needed_globaly = max(len(scope.remain_s) - available_globaly, 0)
                needed = needed_globaly + int(not demoted)
                global_max = (
                    scope.category_free_slots[scope.id_chain] - needed * min_length
                )
            else:
                # As a last resource generator will create
                # only one operation per addresses needed
                remain = sum(scope.free_slots.values()) - min_length * int(not demoted)
                if remain >= len(scope.remain_s):
                    a = ng.choice(from_addresses)
                else:
                    a = ng.choice(scope.remain_s)
                if a in scope.remain_s:
                    scope.remain_s.remove(a)
                needed = len(scope.remain_s) + int(not demoted)
                global_max = sum(scope.free_slots.values()) - needed * min_length
            # Stronger than check performed at the end of generation: assert(not scope.remain_s)
            # assert( global_max >= 0 )
            return a, global_max

        class BaseGenerator:
            def is_available(self, min_length=None):
                if min_length == None:
                    min_length = self.min_length
                available = True
                ge = len(scope.candidates_by_free_slots)
                cores_available = 0
                # Assumes min_length is in non-decreasing order
                if variable_thread_length:
                    available = sum(min_length) <= scope.all_slots
                else:
                    for n_minimum in reversed(min_length):
                        while ge != 0 and n_minimum < ge:
                            ge -= 1
                            cores_available += len(scope.candidates_by_free_slots[ge])
                        cores_available -= 1
                        available = available and cores_available >= 0
                return available

            def satisfies_s_constraint(self):
                after_slots = sum(scope.free_slots.values()) - sum(self.min_length)
                after_remain_s = max(len(scope.remain_s) - self.min_addresses, 0)
                return after_slots >= after_remain_s

            def extend(self, j, a, b, global_max):
                i_weight = self.min_length[-2]
                j_weight = self.min_length[-1]
                assert i_weight <= j_weight
                # (1) Enough shared addresses?
                if scope.s <= 2:
                    return j
                # During extension loop, variable candidates satisfies the follow:
                #           candidates(i+1) subset candidates(i)

                # Breadth selection can be done by choosing a priori which subset of
                # candidates the extension loop will regard. Naturaly this affects
                # the maximal length
                candidates = sum(scope.candidates_by_free_slots[j_weight:], [])
                remove_if_exists(candidates, j)
                available_p = len(candidates)
                # (2) Enough available threads?
                if available_p == 0:
                    return j
                # Prepare to compute max extensions available
                # (a) extensions available from each candidate
                candidates_extensions = sorted(
                    [
                        int(scope.free_slots[k] / scope.extension_min_cost)
                        for k in candidates
                    ]
                )
                # (b) include j on this calculus
                j_slots = int(scope.free_slots[j] / scope.extension_min_cost)
                idx = 0
                while idx != available_p and j_slots > candidates_extensions[idx]:
                    idx += 1
                if idx == available_p:
                    j_slots -= 1
                candidates_extensions.insert(idx, j_slots)
                # (Greedy strategy) Compute max extensions available
                previous = 0
                max_extensions = 0
                while len(candidates_extensions) > 1:
                    quantity = candidates_extensions[0]
                    max_extensions += (quantity - previous) * len(candidates_extensions)
                    while (
                        candidates_extensions and candidates_extensions[0] == quantity
                    ):
                        candidates_extensions.pop(0)
                    previous = quantity
                if candidates_extensions and previous < candidates_extensions[0]:
                    max_extensions += 1
                # Limited by addresses
                if scope.limited_by_mix:
                    max_by_mix = int(
                        scope.category_free_slots[scope.id_chain]
                        / scope.extension_min_cost
                    )
                    max_extensions = min(max_extensions, max_by_mix)
                # Enough extensions?
                if max_extensions == 0:
                    return j
                # Select number of extensions permitted.
                # This number is related to length, but it's not exactly it
                times = ng.randint(0, max_extensions)
                sco_chain = []
                # TODO # Guarantee that the loop bellow will execute
                #       the chosen number of times!
                #        To this end, limit which candidates can be
                #       chosen at certain iteration.
                # First choose the sequence of threads
                seq = [j]
                for _ in range(times):
                    candidates = sum(scope.candidates_by_free_slots[j_weight:], [])
                    remove_if_exists(candidates, j)
                    if len(candidates) == 0:
                        break
                    k = ng.choice(candidates)
                    # demote(j, d_weight) not needed
                    demote(k, j_weight)
                    seq.append(k)
                    j = k
                # Just after this sequence is chosen, decide La <sco La
                j = seq.pop(0)
                for k in seq:
                    sco_chain.append(simulate_add_sco(j, k, True))
                    j = k
                # Delayed construction of chain to solve address assignment
                # TODO # Factor this segment
                if sco_chain:
                    for block in sco_chain[:-1]:
                        c = ng.choice([c for c in scope.shared_addresses if c != b])
                        att_addr(c, block)
                        b = c
                    block = sco_chain[-1]
                    # IMPLEMENTATION ERROR: c != a should be satisfied for
                    # all addresses in the block
                    c = ng.choice(
                        [c for c in scope.shared_addresses if c != b and c != a]
                    )
                    att_addr(c, block)
                return j

        class Category_0(BaseGenerator):
            def __init__(self):
                self.min_length = [1]
                self.min_addresses = 1

            def create_chain(self):
                i = ng.choice(sum(scope.candidates_by_free_slots[1:], []))
                a = ng.choice(scope.shared_addresses)
                if scope.limited_by_mix:
                    avail = min(
                        scope.free_slots[i], scope.category_free_slots[scope.id_chain]
                    )
                else:
                    avail = scope.free_slots[i]
                length = ng.randint(sum(self.min_length), avail)
                demote(i, length)
                Type = None
                for _ in range(length):
                    if scope.last[i, a] != Load:
                        Type = Any()
                    else:
                        Type = Store
                    add_operation(Type, i, a)

        class Category_1(BaseGenerator):
            def __init__(self):
                self.min_length = [1, 2]
                self.min_addresses = 1
                self.breath = 2

            def create_chain(self):
                self.candidates = sum(
                    scope.candidates_by_free_slots[self.min_length[1] :], []
                )
                j = ng.choice(self.candidates)
                self.candidates = sum(
                    scope.candidates_by_free_slots[self.min_length[0] :], []
                )
                self.candidates.remove(j)
                i = ng.choice(self.candidates)
                a = ng.choice(scope.shared_addresses)
                global_max = None
                demote(i, self.min_length[0])
                demote(j, self.min_length[1])
                add_operation(Store, i, a)
                add_operation(Load, j, a)
                add_operation(Any(), j, a)

        class Category_2(BaseGenerator):
            def __init__(self):
                self.min_length = [3, 3]
                self.min_addresses = 2

            def create_chain(self):
                self.candidates = sum(
                    scope.candidates_by_free_slots[self.min_length[1] :], []
                )
                j = ng.choice(self.candidates)

                self.candidates = sum(
                    scope.candidates_by_free_slots[self.min_length[0] :], []
                )
                self.candidates.remove(j)
                i = ng.choice(self.candidates)
                assert scope.s >= 2
                a = ng.choice(scope.shared_addresses)
                b = ng.choice([b for b in scope.shared_addresses if b != a])
                global_max = None
                demote(i, self.min_length[0])
                demote(j, self.min_length[1])
                add_operation(Any(), i, a)
                add_sco(i, j, b, True)
                j = self.extend(j, a, b, global_max)
                add_operation(Any(), j, a, True)

        class Category_3(BaseGenerator):
            def __init__(self):
                self.min_length = [1, 3, 3]
                self.min_addresses = 2

            def is_available(self):
                self.general_case = super().is_available(self.min_length)
                self.special_case = super().is_available(sorted([4, 3]))
                return self.general_case or self.special_case

            def create_chain(self):
                i, j, k = None, None, None
                if self.general_case:
                    self.candidates = sum(
                        scope.candidates_by_free_slots[self.min_length[2] :], []
                    )
                    k = ng.choice(self.candidates)
                    self.candidates = sum(
                        scope.candidates_by_free_slots[self.min_length[1] :], []
                    )
                    self.candidates.remove(k)
                    j = ng.choice(self.candidates)
                    demote(k, self.min_length[2])
                    self.candidates = sum(
                        scope.candidates_by_free_slots[self.min_length[0] :], []
                    )
                    self.candidates.remove(j)
                    i = ng.choice(self.candidates)
                    demote(j, self.min_length[1])
                    demote(i, self.min_length[0])
                else:
                    self.candidates = sum(scope.candidates_by_free_slots[4:], [])
                    k = ng.choice(self.candidates)
                    self.candidates = sum(scope.candidates_by_free_slots[3:], [])
                    self.candidates.remove(k)
                    j = ng.choice(self.candidates)
                    demote(k, self.min_length[2])
                    i = k
                    demote(j, self.min_length[1])
                    demote(i, self.min_length[0])
                assert scope.s >= 2
                a = ng.choice(scope.shared_addresses)
                b = ng.choice([b for b in scope.shared_addresses if b != a])
                global_max = None
                add_sco(i, j, a)
                add_sco(j, k, b, True)
                k = self.extend(k, a, b, global_max)
                add_operation(Load, k, a, True)

        categories = [
            Category_0(),
            Category_1(),
            Category_2(),
            Category_3(),
        ]
        category = None
        while scope.all_slots > 0:
            available = [idx for idx in range(4) if categories[idx].is_available()]
            available_by_mix = [
                idx
                for idx in available
                if scope.category_free_slots[idx] >= sum(categories[idx].min_length)
            ]
            scope.limited_by_mix = len(available_by_mix) != 0
            if len(available_by_mix) != 0:
                available = available_by_mix
            scope.id_chain = self.choice(available)
            category = categories[scope.id_chain]
            category.create_chain()
        return Test(addresses, scope.threads, instances)
