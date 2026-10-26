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

from math import log2

from .._hmv_internal.generator import Chaining, SCA
from .common import BaseGenerator
from .reference.sca import SCA as ReferenceSCA
from hmv import CacheTraits, coverage


class Generator(BaseGenerator):
    name = "SCA"
    description = "Steep-Coverage Ascent Test Generator"
    configuration_key = "SCA"

    def __init__(
        self, memory_traits, cache_traits, thread_count, metrics, configs, **options
    ):
        super().__init__(memory_traits, cache_traits, thread_count, metrics)
        self.cache_traits = CacheTraits(
            configs.get("cache_size", self.cache_traits.size),
            configs.get("block_size", self.cache_traits.block_size),
            configs.get("cache_associativity", self.cache_traits.associativity),
        )
        self.sbc = configs["sbc"]
        self.mix = configs["mix"]
        self.variant = SCA.Variant[options["variant"]]
        self.n_range = super().make_range(options["nmin"], options["nmax"])
        self.s_range = super().make_range(options["smin"], options["smax"])
        self.abc = options["abc"]
        self.instances = options["instances"]
        self.reference_implementation = options["reference_implementation"]
        self.sca = None
        self.n = 0
        self.s = 0
        self.k = 0
        assert self.n_range.is_power2(), "test size range should be valid powers of 2"
        assert self.s_range.is_power2(), "address range should be a valid powers of 2"

    def __call__(self):
        test = self.sca()
        self.n, self.s, self.k = self.sca.constraints()
        return test

    def feedback(self, output):
        return self.sca.feedback(output)

    def fields(self):
        return ["n", "s", "k"]

    def field_values(self):
        return {"n": self.n, "s": self.s, "k": self.k}

    def max_address_count(self):
        return self.s_range.max()

    def max_thread_size(self):
        return self.n_range.max() // self.thread_count

    def parameters(self):
        return {
            "variant": self.variant.name,
            "nmin": self.n_range.min(),
            "nmax": self.n_range.max(),
            "smin": self.s_range.min(),
            "smax": self.s_range.max(),
            "abc": self.abc,
            "instances": self.instances,
        }

    def set_seed(self, seed):
        if self.reference_implementation:
            self.sca = ReferenceSCA(
                self.variant,
                self.memory_traits,
                self.cache_traits,
                self.thread_count,
                self.n_range.to_exponent_range(),
                self.s_range.to_exponent_range(),
                self.abc,
                self.sbc,
                self.instances,
                self.mix,
                seed,
            )
        else:
            self.sca = SCA(
                self.variant,
                self.memory_traits,
                self.cache_traits,
                self.thread_count,
                self.n_range.to_exponent_range(),
                self.s_range.to_exponent_range(),
                self.abc,
                self.sbc,
                self.instances,
                Chaining.Mix(self.mix),
                seed,
            )

    def test_display(self):
        return "n = {}, s = {}, k = {}".format(self.n, self.s, self.k)

    def unique_id(self):
        return "{}.{}.n{}.s{}.a{}.i{}".format(
            self.name,
            self.variant.name,
            "{}_{}".format(self.n_range.min(), self.n_range.max()),
            "{}_{}".format(self.s_range.min(), self.s_range.max()),
            self.abc,
            self.instances,
        )

    @classmethod
    def define_options(cls, parser):
        cls.define_n_range_options(parser)
        cls.define_s_range_options(parser)
        cls.define_instances_option(parser)
        parser.add_argument(
            "-v",
            "--variant",
            required=True,
            choices=[v.name for v in SCA.Variant],
            help="search space variant",
        )
        parser.add_argument(
            "-a",
            "--abc",
            type=int,
            default=6,
            help="alignment biasing constraint (bits)",
        )
        parser.add_argument(
            "-R",
            "--reference-implementation",
            action="store_true",
            help="use the reference implementation (made in pure python)",
        )


__all__ = [Generator]
