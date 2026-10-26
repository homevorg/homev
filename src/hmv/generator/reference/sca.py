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

from collections import defaultdict
from math import ceil, log2

from .common import PAC, Constraints, list_range, standard_space
from .component.chaining import Generator as TestGenerator


class SCA:
    def __init__(
        self,
        variant,
        memory_traits,
        cache_traits,
        thread_count,
        n_range,
        s_range,
        abc,
        sbc,
        instances,
        mix,
        seed,
    ):
        self.variant = variant
        self.memory_traits = memory_traits
        self.cache_traits = cache_traits
        self.offset_bits = self.cache_traits.offset_size()
        self.index_bits = self.cache_traits.index_size()
        self.tag_bits = (
            self.memory_traits.address_size - self.index_bits - self.offset_bits
        )
        self.thread_count = thread_count
        self.N = list_range(n_range)
        self.S = list_range(s_range)
        self.abc = abc
        self.sbc = sbc
        self.instances = instances
        self.seed = seed
        self.G = standard_space(
            self.N,
            self.S,
            PAC(
                self.tag_bits,
                self.index_bits,
                self.offset_bits,
                self.sbc,
            ),
            self.variant.name,
        )
        self.categories_mix = mix
        self.mode = 0
        self.N = set()
        self.P = defaultdict(set)
        for cstrs in self.G:
            self.N.add(cstrs.n)
            self.P[cstrs.n].add(cstrs)
        self.n = min(self.N)
        self.test_generator = TestGenerator()

    def __call__(self):
        if not self.P[self.n]:
            self.N.remove(self.n)
            if not self.N:
                return None
            self.n = min(self.N)
        self.s, self.k = self._apply_coverage_model(self.P[self.n])
        self.P[self.n].remove(Constraints(self.n, self.s, self.k))
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
        return output.aggregated_metrics[0].full()

    def constraints(self):
        return self.n, self.s, self.k

    def _apply_coverage_model(self, P):
        Pc = set([cstrs for cstrs in P if cstrs.k != 1])
        if self.mode == 0 or not Pc:
            k = min(cstrs.k for cstrs in P)
            s = max(cstrs.s for cstrs in P if cstrs.k == k)
            self.mode = 1
        else:
            s = min(cstrs.s for cstrs in P if cstrs.k != 1)
            k = max(cstrs.k for cstrs in P if cstrs.s == s)
            self.mode = 0
        return s, k
