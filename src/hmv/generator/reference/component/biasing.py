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

""" Address Assigment Module """

import bisect
import collections
import math
import random as prng

from .common import Hash, BaseGenerator


class InternalAddressGenerator:
    def __init__(
        self,
        tag_size_bits,
        index_size_bits,
        cacheline_size,
        alignment,
        true_sharing,
        word_size,
        generator,
    ):
        # Specification
        self.tag_size_bits = tag_size_bits
        self.index_size_bits = index_size_bits
        self.cacheline_size = cacheline_size
        self.alignment = alignment
        self.word_size = word_size
        # Derived spec
        self.quantity_of = dict()
        self.quantity_of["tag"] = 2**tag_size_bits
        self.quantity_of["index"] = 2**index_size_bits
        offset_size_bits = int(math.log(cacheline_size, 2))
        target_offset_size_bits = offset_size_bits - alignment
        self.quantity_of["offset"] = 2**target_offset_size_bits
        # Random Number Generator
        self.ng = generator
        # Candidates
        self.candidates = dict()
        self.candidates["tag"] = [0, self.quantity_of["tag"]]
        self.candidates["index"] = [0, self.quantity_of["index"]]
        self.candidates["offset"] = [0, self.quantity_of["offset"]]
        assertion_msg = "alignment ({0}) must be less or equal to cacheline_size ({1})!"
        assert alignment <= offset_size_bits, assertion_msg.format(
            alignment, cacheline_size
        )
        self.true_sharing = true_sharing
        self.debug_competition_pattern = collections.Counter()
        self.debug_uniqueness = collections.Counter()
        self.debug_sharing = collections.Counter()

    def select(self, field, modifier, ignore):
        interval = self.candidates[field]
        a, b = (interval[0] + modifier[0], interval[1] + modifier[1])
        ignore = sorted(ignore)
        ia = bisect.bisect_left(ignore, a)
        ib = len(ignore)
        while ia < ib and ignore[ib - 1] >= interval[1]:
            ib = ib - 1
        while ia < ib and ignore[ib - 1] >= b:
            b = b - 1
            ib = ib - 1
        max_value = b - len(ignore[ia:ib])
        assert (
            a <= max_value
        ), "It is impossible to satisfy Alignment biasing constraint!"
        assert field == "offset" or a < max_value
        if a == max_value:
            value = 0
        else:
            value = self.ng.randrange(a, max_value)
            for spent_value in ignore[ia:ib]:
                if value >= spent_value:
                    value += 1
                else:
                    break
        return value

    def construct(self, tag, index, offset, base=0):
        conclude = dict()
        conclude["tag"] = lambda value: value
        conclude["index"] = lambda value: value
        conclude["offset"] = lambda value: value * (2**self.alignment)
        assert (
            0 <= offset and offset < self.quantity_of["offset"]
        ), "Incorrect OFFSET(1)"
        tag = conclude["tag"](tag)
        index = conclude["index"](index)
        offset = conclude["offset"](offset)
        assert 0 <= tag and tag < self.quantity_of["tag"], "Incorrect TAG!"
        assert 0 <= index and index < self.quantity_of["index"], "Incorrect INDEX!"
        assert 0 <= offset and offset < self.cacheline_size, "Incorrect OFFSET(2)!"
        assert (offset % (2**self.alignment)) == 0, "Alignment Constraint broken!"
        address = (
            (tag) * (2**self.index_size_bits) + (index)
        ) * self.cacheline_size + (offset)
        self.debug_competition_pattern[index] += 1
        self.debug_uniqueness[address] += 1
        self.debug_sharing[tag, index] += 1
        assert self.debug_uniqueness[address] == 1, "Uniqueness Constraint broken!"
        assert (
            not self.true_sharing or self.debug_sharing[tag, index]
        ), "Sharing Constraint broken!"
        return base + address

    def validate(self, competition_pattern):
        CP = convert_to_list(competition_pattern)
        assert (
            sorted(self.debug_competition_pattern.values()) == CP
        ), "Competition Constraint broken!"


def select_addresses(address_specs, competition_pattern, true_sharing, word_size=4):
    A = []
    tags_limit = address_specs.quantity_of["offset"]
    I = collections.defaultdict(list)
    modifier = dict()
    modifier["tag"] = [0, 0]
    modifier["offset"] = [0, 0]
    for cardinality, quantity in enumerate(competition_pattern):
        modifier["index"] = [0, -quantity]
        for i in range(quantity):
            assertion_msg = "implementation assumption that all members of a competition_pattern have cardinality greater than 0!"
            assert cardinality > 0, assertion_msg
            modifier["index"][1] += 1
            index = address_specs.select("index", modifier["index"], ignore=I["index"])
            I["tag"] = list()
            I["offset"] = collections.defaultdict(list)
            for symbolic_addresss in range(cardinality):
                tag = address_specs.select("tag", modifier["tag"], ignore=I["tag"])
                offset = address_specs.select(
                    "offset", modifier["offset"], ignore=I["offset"][tag]
                )
                # Uniqueness Constraint: no two addresses in A are equal
                A.append(address_specs.construct(tag, index, offset))
                # To respect uniqueness constraint
                I["offset"][tag].append(offset)
                # To respect sharing constraint and uniqueness constraint
                if true_sharing or len(I["offset"][tag]) == tags_limit:
                    I["tag"].append(tag)
            I["index"].append(index)
            modifier["index"][0] = index + 1
    address_specs.validate(competition_pattern)
    return A


# Convert a Characterization Function of a given competition pattern
# to a list of cardinalities of each edge-disjoint subgraph
def convert_to_list(competition_pattern):
    def conversion():
        for cardinality, quantity in enumerate(competition_pattern):
            for i in range(quantity):
                yield cardinality

    return list(conversion())


# Biased Address Assignment
class Generator(BaseGenerator):
    def __init__(
        self,
        seed,
        tag_size_bits,
        index_size_bits,
        cacheline_size,
        alignment,
        clique_number,
        chromatic_number,
        false_sharing,
        word_size,
    ):
        self.seed = Hash(seed, clique_number, chromatic_number)
        self.tag_size_bits = tag_size_bits
        self.index_size_bits = index_size_bits
        self.cacheline_size = cacheline_size
        self.alignment = alignment
        self.clique_number = clique_number
        self.chromatic_number = chromatic_number
        self.true_sharing = not false_sharing
        self.word_size = word_size
        K = self.clique_number
        X = self.chromatic_number
        self.cps_memory = dict()
        # (K,S) is restricted by constraints over (tag,index,offset)
        index_qtd = 2**self.index_size_bits
        assertion_msg = "clique number ({0}) too big for index_size: K should be less or equal to {1}!"
        assert self.clique_number <= index_qtd, assertion_msg.format(K, index_qtd)
        tag_qtd = 2**self.tag_size_bits
        assertion_msg = "chromatic number ({0}) too big for tag_size: X should be less or equal to {1} (when true_sharing)!"
        assert false_sharing or self.chromatic_number <= tag_qtd, assertion_msg.format(
            X, tag_qtd
        )
        offset_size_bits = int(math.log(cacheline_size, 2))
        assertion_msg = "alignment ({0}) must be less or equal to cacheline_size ({1})!"
        assert alignment <= offset_size_bits, assertion_msg.format(
            alignment, cacheline_size
        )
        offset_qtd = 2 ** (offset_size_bits - self.alignment)
        assertion_msg = "chromatic number ({0}) too big for tag_size: X should be less or equal to {1} (when false_sharing)!"
        assert self.true_sharing or self.chromatic_number <= (
            tag_qtd + offset_qtd
        ), assertion_msg.format(X, tag_qtd)

    def select(self, shared_addresses):
        self.ng = prng.Random(Hash(self.seed, shared_addresses))
        address_specs = InternalAddressGenerator(
            self.tag_size_bits,
            self.index_size_bits,
            self.cacheline_size,
            self.alignment,
            self.true_sharing,
            self.word_size,
            generator=self.ng,
        )
        competition_pattern = self.select_competition_pattern(
            self.clique_number,
            self.chromatic_number,
            shared_addresses,
            generator=self.ng,
        )
        return select_addresses(
            address_specs, competition_pattern, self.true_sharing, self.word_size
        )

    def select_competition_pattern(
        self, clique_number, chromatic_number, shared_addresses, generator
    ):
        k, x, s = clique_number, chromatic_number, shared_addresses
        if (k, x, s) in self.cps_memory:
            cps = self.cps_memory[k, x, s]
        elif (k * x) == s:
            cps = [[k if xj == x else 0 for xj in range(chromatic_number + 1)]]
            self.cps_memory[k, x, s] = cps
        else:
            cps = set()
            minimum_shared_addresses = clique_number - 1 + chromatic_number
            restriction = minimum_shared_addresses <= shared_addresses
            assertion_msg = "Minimum number of shared_addresses to induce a competition pattern with ({},{}) is {}!"
            assert restriction, assertion_msg.format(
                clique_number, chromatic_number, minimum_shared_addresses
            )
            restriction = chromatic_number > 0
            assertion_msg = "Chromatic number must be greater than 0!"
            assert restriction, assertion_msg
            # competition_pattern must satisfy chromatic number
            f = [0 for xj in range(chromatic_number + 1)]
            clique_number = clique_number - 1
            shared_addresses = shared_addresses - chromatic_number
            f[chromatic_number] += 1
            # Base case for induction
            shared_addresses = shared_addresses - clique_number
            f[1] += clique_number
            cps.add(tuple(f))
            # Inductive Step: (k,x,s) --> (k,x,s+1)
            for _ in range(shared_addresses):
                new_cps = set()
                for f in cps:
                    for xj in range(1, chromatic_number):
                        new_f = list(f)
                        if new_f[xj] > 0:
                            new_f[xj] -= 1
                            new_f[xj + 1] += 1
                            new_cps.add(tuple(new_f))
                cps = set(new_cps)
            cps = sorted(cps)
            assert (
                len(cps) > 0
            ), "No competition_pattern is induced by ({},{},{})!".format(k, x, s)
            self.cps_memory[k, x, s] = cps
        chosen = generator.randrange(len(cps))
        return cps[chosen]
