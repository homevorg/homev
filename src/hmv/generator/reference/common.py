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

from collections import namedtuple
from math import ceil

# Biasing Constraints
Constraints = namedtuple("Constraints", "n,s,k")
# Physical Address Constraints
PAC = namedtuple("PAC", "tag, index, offset, false_sharing")


def _is_physically_possible(cstrs, pac):
    # compute the chromatic number
    x = ceil(cstrs.s / cstrs.k)
    # check if clique number (k) is possible (<= number of assoc. sets)
    k_posle = cstrs.k <= 2 ** (pac.index)
    if pac.false_sharing:
        # if false sharing is allowed ...
        # physical addresses with the same index may have distinct tags
        n_bits_same_index = pac.tag + pac.offset
    else:
        # if true sharing is enforced ...
        # physical addresses with the same index have distinct tags
        n_bits_same_index = max(pac.tag, pac.offset)
    x_posle = x <= 2 ** (n_bits_same_index)
    n_bits = n_bits_same_index + pac.index
    n_physical_addresses = 2 ** (n_bits)
    s_posle = cstrs.s <= n_physical_addresses
    return s_posle and k_posle and x_posle


def _is_valid(cstrs, pac):
    return (
        (1 <= cstrs.k)
        and (cstrs.k <= cstrs.s)
        and (cstrs.s <= cstrs.n)
        and _is_physically_possible(cstrs, pac)
    )


def _check_and_add(P, cstrs, pac):
    if _is_valid(cstrs, pac):
        P.add(cstrs)


def _standard_plane(n, S, pac, variant):
    P = set()
    if variant == "EFFORT":
        for s in S:
            _check_and_add(P, Constraints(n, s, 1), pac)
    elif variant == "COVERAGE":
        for s in S:
            K = [k for k in range(1, s + 1) if s % k == 0]
            for k in K:
                _check_and_add(P, Constraints(n, s, k), pac)
    elif variant == "TRADEOFF":
        for s in S:
            _check_and_add(P, Constraints(n, s, 1), pac)
            _check_and_add(P, Constraints(n, s, s), pac)
    return P


def standard_space(N, S, pac, variant="COVERAGE"):
    G = set()
    for n in N:
        G = G.union(_standard_plane(n, S, pac, variant))
    return G


def list_range(r):
    return list(2**e for e in range(r.min(), r.max() + 1))
