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

import random as prng


def Hash(*args):
    hashed_seed = 0
    for argument in args:
        value = argument if type(argument) is int else int(argument * 100)
        hashed_seed = hashed_seed * 37 + value
    return hashed_seed


class BaseGenerator(object):
    def randint(self, a, b):
        return self.ng.randint(a, max(a, b))

    def randrange(self, stop):
        return self.ng.randrange(stop)

    def choice(self, universe, p_distr=None, total=1.0):
        n = len(universe)

        p = lambda x: (1 / n)
        if p_distr != None:
            p = lambda x: (p_distr[x] / total)

        prob = self.ng.random()
        acc = 0

        for idx in range(n):
            acc = acc + p(idx)
            if prob < acc:
                return universe[idx]
        return universe[-1]

    def next_index(self, idx, s, deterministic=False):
        if deterministic:
            idx = (idx + 1) % s
        elif self.remain_n >= len(self.remain_s):
            idx = self.ng.randrange(s)
        else:
            idx = self.choice(self.remain_s)

        if idx in self.remain_s:
            self.remain_s.remove(idx)

        return idx
