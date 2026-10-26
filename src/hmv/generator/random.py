# Copyright (C) 2025-2026 Marleson Graf

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

from .._hmv_internal import RandomOperationMix
from .._hmv_internal.generator import (
    Biasing,
    Chaining,
    RandomAddress,
    RandomThread,
)
from .common import BaseGenerator
from hmv import CacheTraits, MemoryTraits, Test


def _float_to_str(f):
    return str(f).split(".")[-1] if f != None else "R"


class _RandomThreadWrapper:
    def __init__(self, thread_count, n_range, **options):
        self.thread_count = thread_count
        self.n_range = n_range
        self.uniform_threads = options["uniform_threads"]
        self.load_ratio = options["load_ratio"]
        self.store_ratio = options["store_ratio"]
        self.fence_ratio = options["fence_ratio"]
        self.nop_ratio = options["nop_ratio"]

    def fields(self):
        return ["loads", "stores", "fences", "nops"]

    def field_values(self):
        return {
            "loads": self.loads,
            "stores": self.stores,
            "fences": self.fences,
            "nops": self.nops,
        }

    def __call__(self, s):
        threads = self.generator(self.n_range, s)
        self.loads, self.stores, self.fences, self.nops = self.generator.mix().ratios
        return threads

    def set_seed(self, seed):
        self.generator = RandomThread(
            self.thread_count,
            self.uniform_threads,
            RandomOperationMix(
                [
                    self.load_ratio,
                    self.store_ratio,
                    self.fence_ratio,
                    self.nop_ratio,
                ]
            ),
            seed,
        )

    def max_thread_size(self):
        return (
            self.n_range.max() // self.thread_count
            if self.uniform_threads
            else self.n_range.max()
        )

    def parameters(self):
        params = {"uniform_threads": self.uniform_threads}
        if self.load_ratio:
            params["load_ratio"] = self.load_ratio
        if self.store_ratio:
            params["store_ratio"] = self.store_ratio
        if self.fence_ratio:
            params["fence_ratio"] = self.fence_ratio
        if self.nop_ratio:
            params["nop_ratio"] = self.nop_ratio
        return params

    def test_display(self):
        return "mix: [{:.2f}, {:.2f}, {:.2f}, {:.2f}]".format(
            self.loads,
            self.stores,
            self.fences,
            self.nops,
        )

    def unique_id(self):
        return "u{}.l{}_s{}_m{}_n{}".format(
            self.uniform_threads,
            _float_to_str(self.load_ratio),
            _float_to_str(self.store_ratio),
            _float_to_str(self.fence_ratio),
            _float_to_str(self.nop_ratio),
        )

    @classmethod
    def define_options(cls, parser):
        parser.add_argument(
            "-u",
            "--uniform-threads",
            action="store_true",
            help="use uniform thread size",
        )
        parser.add_argument(
            "--load-ratio",
            type=float,
            help="use fixed ratio of load operations",
        )
        parser.add_argument(
            "--store-ratio",
            type=float,
            help="use fixed ratio of store operations",
        )
        parser.add_argument(
            "--fence-ratio",
            type=float,
            help="use fixed ratio of fence operations",
        )
        parser.add_argument(
            "--nop-ratio",
            type=float,
            help="use fixed ratio of nop operations",
        )


class _ChainingWrapper:
    def __init__(self, thread_count, n_range, **options):
        self.thread_count = thread_count
        self.n_range = n_range
        self.cat0_ratio = options["cat0_ratio"]
        self.cat1_ratio = options["cat1_ratio"]
        self.cat2_ratio = options["cat2_ratio"]
        self.cat3_ratio = options["cat3_ratio"]

    def fields(self):
        return ["cat0", "cat1", "cat2", "cat3"]

    def field_values(self):
        return {
            "cat0": self.cat0,
            "cat1": self.cat1,
            "cat2": self.cat2,
            "cat3": self.cat3,
        }

    def __call__(self, s):
        threads = self.generator(self.n_range, s)
        self.cat0, self.cat1, self.cat2, self.cat3 = self.generator.mix().ratios
        return threads

    def set_seed(self, seed):
        self.generator = Chaining(
            self.thread_count,
            Chaining.RandomMix(
                [self.cat0_ratio, self.cat1_ratio, self.cat2_ratio, self.cat3_ratio]
            ),
            seed,
        )

    def max_thread_size(self):
        return self.n_range.max() // self.thread_count

    def parameters(self):
        params = dict()
        if self.cat0_ratio:
            params["cat0_ratio"] = self.cat0_ratio
        if self.cat1_ratio:
            params["cat1_ratio"] = self.cat1_ratio
        if self.cat2_ratio:
            params["cat2_ratio"] = self.cat2_ratio
        if self.cat2_ratio:
            params["cat2_ratio"] = self.cat2_ratio
        return params

    def test_display(self):
        return "mix: [{:.2f}, {:.2f}, {:.2f}, {:.2f}]".format(
            self.cat0, self.cat1, self.cat2, self.cat3
        )

    def unique_id(self):
        return "c{}_{}_{}_{}".format(
            _float_to_str(self.cat0_ratio),
            _float_to_str(self.cat1_ratio),
            _float_to_str(self.cat2_ratio),
            _float_to_str(self.cat3_ratio),
        )

    @classmethod
    def define_options(cls, parser):
        parser.add_argument(
            "--cat0-ratio",
            type=float,
            help="use fixed ratio of category 0",
        )
        parser.add_argument(
            "--cat1-ratio",
            type=float,
            help="use fixed ratio of category 1",
        )
        parser.add_argument(
            "--cat2-ratio",
            type=float,
            help="use fixed ratio of category 2",
        )
        parser.add_argument(
            "--cat3-ratio",
            type=float,
            help="use fixed ratio of category 3",
        )


class _RandomAddressWrapper:
    def __init__(self, memory_traits, s_range):
        self.memory_traits = memory_traits
        self.s_range = s_range

    def fields(self):
        return []

    def field_values(self):
        return dict()

    def __call__(self):
        return self.generator(self.s_range)

    def set_seed(self, seed):
        self.generator = RandomAddress(self.memory_traits, seed)

    def parameters(self):
        return dict()

    def test_display(self):
        return ""

    def unique_id(self):
        return ""

    @classmethod
    def define_options(cls, parser):
        pass


class _BiasingWrapper:
    def __init__(self, configs, memory_traits, cache_traits, s_range, **options):
        self.memory_traits = memory_traits
        self.cache_traits = CacheTraits(
            configs.get("cache_size", cache_traits.size),
            configs.get("block_size", cache_traits.block_size),
            configs.get("cache_associativity", cache_traits.associativity),
        )
        self.sbc = configs["sbc"]
        self.s_range = s_range
        self.fixed_abc = options["abc"]

    def fields(self):
        return ["k", "x", "abc"]

    def field_values(self):
        return {"k": self.cbc.k, "x": self.cbc.x, "abc": self.abc}

    def __call__(self):
        if self.fixed_abc:
            addresses = self.generator(self.s_range, self.fixed_abc)
        else:
            addresses = self.generator(self.s_range)
        self.cbc = self.generator.cbc()
        self.abc = self.generator.abc()
        return addresses

    def set_seed(self, seed):
        self.generator = Biasing(self.memory_traits, self.cache_traits, self.sbc, seed)

    def parameters(self):
        if self.fixed_abc:
            return {"fixed_abc": self.fixed_abc}
        return dict()

    def test_display(self):
        return "k: {}, x: {}, abc: {}".format(self.cbc.k, self.cbc.x, self.abc)

    def unique_id(self):
        return "a{}".format(self.fixed_abc) if self.fixed_abc else "aR"

    @classmethod
    def define_options(cls, parser):
        parser.add_argument(
            "-a",
            "--abc",
            type=int,
            help="use fixed alignment biasing constraint (bits)",
        )


class _Random(BaseGenerator):
    configuration_key = "Random"

    def __init__(self, memory_traits, cache_traits, thread_count, metrics, **options):
        super().__init__(memory_traits, cache_traits, thread_count, metrics)
        self.n_range = super().make_range(options["nmin"], options["nmax"])
        self.s_range = super().make_range(options["smin"], options["smax"])
        self.instances = options["instances"]

    def __call__(self):
        addresses = self.address_generator()
        self.s = len(addresses)
        threads = self.thread_generator(self.s)
        self.n = sum([len(thread) for thread in threads])
        return Test(addresses, threads, self.instances)

    def feedback(self, output):
        return False

    def fields(self):
        return (
            ["n", "s"]
            + self.thread_generator.fields()
            + self.address_generator.fields()
        )

    def field_values(self):
        return (
            {"n": self.n, "s": self.s}
            | self.thread_generator.field_values()
            | self.address_generator.field_values()
        )

    def max_address_count(self):
        return self.s_range.max()

    def max_thread_size(self):
        return self.thread_generator.max_thread_size()

    def parameters(self):
        return (
            {
                "nmin": self.n_range.min(),
                "nmax": self.n_range.max(),
                "smin": self.s_range.min(),
                "smax": self.s_range.max(),
                "instances": self.instances,
            }
            | self.thread_generator.parameters()
            | self.address_generator.parameters()
        )

    def set_seed(self, seed):
        self.thread_generator.set_seed(seed)
        self.address_generator.set_seed(seed)

    def test_display(self):
        base_id = "n: {}, s: {}".format(self.n, self.s)
        if thread_display := self.thread_generator.test_display():
            base_id += ", " + thread_display
        if address_display := self.address_generator.test_display():
            base_id += ", " + address_display
        return base_id

    def unique_id(self):
        base_id = "{}.n{}.s{}.i{}".format(
            self.name,
            "{}_{}".format(self.n_range.min(), self.n_range.max()),
            "{}_{}".format(self.s_range.min(), self.s_range.max()),
            self.instances,
        )
        if thread_id := self.thread_generator.unique_id():
            base_id += "." + thread_id
        if address_id := self.address_generator.unique_id():
            base_id += "." + address_id
        return base_id

    @classmethod
    def define_options(cls, parser):
        cls.define_n_range_options(parser)
        cls.define_s_range_options(parser)
        cls.define_instances_option(parser)


class RnR(_Random):
    name = "RnR"
    description = "Full Random Generator"

    def __init__(
        self, memory_traits, cache_traits, thread_count, metrics, configs, **options
    ):
        super().__init__(memory_traits, cache_traits, thread_count, metrics, **options)
        self.thread_generator = _RandomThreadWrapper(
            thread_count, self.n_range, **options
        )
        self.address_generator = _RandomAddressWrapper(self.memory_traits, self.s_range)

    @classmethod
    def define_options(cls, parser):
        super().define_options(parser)
        _RandomThreadWrapper.define_options(parser)
        _RandomAddressWrapper.define_options(parser)


class RnB(_Random):
    name = "RnB"
    description = "Random Biasing Generator"

    def __init__(
        self, memory_traits, cache_traits, thread_count, metrics, configs, **options
    ):
        super().__init__(memory_traits, cache_traits, thread_count, metrics, **options)
        self.thread_generator = _RandomThreadWrapper(
            thread_count, self.n_range, **options
        )
        self.address_generator = _BiasingWrapper(
            configs, self.memory_traits, self.cache_traits, self.s_range, **options
        )

    @classmethod
    def define_options(cls, parser):
        super().define_options(parser)
        _RandomThreadWrapper.define_options(parser)
        _BiasingWrapper.define_options(parser)


class CnR(_Random):
    name = "CnR"
    description = "Chaining Random Generator"

    def __init__(
        self, memory_traits, cache_traits, thread_count, metrics, configs, **options
    ):
        super().__init__(memory_traits, cache_traits, thread_count, metrics, **options)
        self.thread_generator = _ChainingWrapper(thread_count, self.n_range, **options)
        self.address_generator = _RandomAddressWrapper(self.memory_traits, self.s_range)

    @classmethod
    def define_options(cls, parser):
        super().define_options(parser)
        _ChainingWrapper.define_options(parser)
        _RandomAddressWrapper.define_options(parser)


class CnB(_Random):
    name = "CnB"
    description = "Chaining & Biasing Generator"

    def __init__(
        self, memory_traits, cache_traits, thread_count, metrics, configs, **options
    ):
        super().__init__(memory_traits, cache_traits, thread_count, metrics, **options)
        self.thread_generator = _ChainingWrapper(thread_count, self.n_range, **options)
        self.address_generator = _BiasingWrapper(
            configs, self.memory_traits, self.cache_traits, self.s_range, **options
        )

    @classmethod
    def define_options(cls, parser):
        super().define_options(parser)
        _ChainingWrapper.define_options(parser)
        _BiasingWrapper.define_options(parser)
