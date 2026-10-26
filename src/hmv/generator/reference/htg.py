# Copyright (c) 2025-2026 Marleson Graf
# Copyright (c) 2018-2025 Gabriel Arthur Gerber Andrade

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

from random import Random
from math import log2, ceil

from .common import PAC, Constraints, list_range, standard_space
from .component.chaining import Generator as TestGenerator
from hmv import coverage


class HTG:
    def __init__(
        self,
        memory_traits,
        cache_traits,
        thread_count,
        metric,
        n_range,
        s_range,
        abc,
        sbc,
        instances,
        mix,
        seed,
    ):
        self.memory_traits = memory_traits
        self.cache_traits = cache_traits
        self.offset_bits = self.cache_traits.offset_size()
        self.index_bits = self.cache_traits.index_size()
        self.tag_bits = (
            self.memory_traits.address_size - self.index_bits - self.offset_bits
        )
        self.thread_count = thread_count
        self.metric = metric
        self.abc = abc
        self.sbc = sbc
        self.instances = instances
        self.seed = seed
        self.urng = Random(self.seed)
        self.categories_mix = mix
        self._initialize_spaces(list_range(n_range), list_range(s_range))
        self.test_generator = TestGenerator()
        self.total_coverage = coverage.Metric(self.metric)
        self.results = dict()
        # set variable for alternating between classes col. and repl.
        self.mode = 0

    def __call__(self):
        if not self.pool:
            return None
        self.n, self.s, self.k = self._apply_coverage_model()
        return self.test_generator.generate_program(
            self.seed,
            self.thread_count,
            self.n / self.thread_count,
            self.s,
            self.categories_mix,
            tag_size_bits=self.tag_bits,
            index_size_bits=self.index_bits,
            cacheline_size=self.cache_traits.block_size,
            alignment=self.abc,
            clique_number=self.k,
            chromatic_number=ceil(self.s / self.k),
            false_sharing=self.sbc,
            word_size=self.memory_traits.word_size.value,
            instances=self.instances,
        )

    def feedback(self, output):
        # algorithm 6: driver
        cstrs = Constraints(self.n, self.s, self.k)
        # update explored constraints
        self.V.add(cstrs)
        # update coverage model plane
        self.P.remove(Constraints(None, self.s, self.k))
        # update remaining constraints
        self.pool.remove(cstrs)
        # store coverage result
        self.results[cstrs] = output.metrics[0].clone()
        # compute total coverage
        self.total_coverage.merge(output.metrics[0])
        return self._explore()

    def constraints(self):
        return self.n, self.s, self.k

    def _initialize_spaces(self, N, S):
        # algorithm 1: directing engine
        self.space = standard_space(
            N,
            S,
            PAC(
                self.tag_bits,
                self.index_bits,
                self.offset_bits,
                self.sbc,
            ),
        )
        self.V = set()
        # select smallest n
        n = min(N)
        # select a random number of shared locations
        s = self.urng.choice(S)
        # compute the initial solution with selected n and s
        self.G = set(cstrs for cstrs in self.space if cstrs.n == n and cstrs.s <= s)
        self.P = set()
        self.Q = set()
        # start exploring all constraints from the initial solution
        self.pool = set(self.G)

    def _apply_coverage_model(self):
        # algorithm 6: driver
        if not self.P:
            # define (s, k)-pairs yet to be explored
            self.P = set(Constraints(None, cstrs.s, cstrs.k) for cstrs in self.pool)
        if self.mode == 0:
            k = min(cstrs.k for cstrs in self.P)
            s = max(cstrs.s for cstrs in self.P if cstrs.k == k)
            self.mode = 1
        else:
            s = min(cstrs.s for cstrs in self.P)
            k = max(cstrs.k for cstrs in self.P if cstrs.s == s)
            self.mode = 0
        n = min([cstrs.n for cstrs in self.pool if cstrs.s == s and cstrs.k == k])
        return n, s, k

    def _explore(self):
        # algorithm 2: explorer
        # generate new neighbor once Q is fully explored
        if not self.pool:
            self.G = self._reduce_neighbor()
            # stop on full space explored
            if not self.space - self.V:
                return True
            self.Q = self._generate_neighbor(self.G)
            if not (self.Q - self.V):
                self.Q = self._generate_neighbor(self.V)
            self.pool = self.Q - self.V
        # stop on full coverage
        return self.total_coverage.full()

    def _generate_neighbor(self, G):
        # algorithm 4: generate neighbor
        Q = set(p for cstrs in G for p in self._pi(cstrs) if p in self.space)
        # select a random sample of neighbor
        return self._random_sample(Q)

    def _pi(self, cstrs):
        # algorithm 5: select neighboring points
        return [
            Constraints(cstrs.n << 1, cstrs.s, cstrs.k),
            Constraints(cstrs.n >> 1, cstrs.s, cstrs.k),
            Constraints(cstrs.n, cstrs.s << 1, cstrs.k),
            Constraints(cstrs.n, cstrs.s >> 1, cstrs.k),
            Constraints(cstrs.n, cstrs.s, cstrs.k << 1),
            Constraints(cstrs.n, cstrs.s, cstrs.k >> 1),
        ]

    def _reduce_neighbor(self):
        # algorithm 3: reduce neighbor
        # if no neighbor to reduce, return current solution
        if not self.Q:
            return self.G
        # combine solution and neighbor
        combined = self.G | self.Q
        # select a subset that maintains the coverage
        Gl = set(combined)
        cv = self._coverage_of(combined)
        while combined:
            cstrs = self.urng.choice(list(combined))
            Gl.remove(cstrs)
            if self._coverage_of(Gl) < cv:
                Gl.add(cstrs)
            combined.remove(cstrs)
        return Gl

    def _coverage_of(self, G):
        cov = coverage.Metric(self.metric)
        for cstrs in G:
            cov.merge(self.results[cstrs])
        return cov.value()

    def _random_sample(self, L):
        sample = set()
        # randomly select elements from L
        for element in L:
            if self.urng.getrandbits(1):
                sample.add(element)
        # if no unexplored constraints were selected
        if not sample - self.V:
            # add one random unexplored constraint if there is any
            if unexplored := list(L - self.V):
                sample.add(self.urng.choice(unexplored))
        return sample
