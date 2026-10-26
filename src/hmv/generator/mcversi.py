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

from .._hmv_internal.generator import McVerSi
from .common import BaseGenerator
from hmv import Monitor, Operation, TraceSelection


class Generator(BaseGenerator):
    name = "McVerSi"
    description = "McVerSi Generator"
    configuration_key = "McVerSi"

    def __init__(
        self, memory_traits, cache_traits, thread_count, metrics, configs, **options
    ):
        super().__init__(memory_traits, cache_traits, thread_count, metrics)
        self.tournament_size = configs["tournament_size"]
        self.mates = configs["mates"]
        self.mutation_rate = configs["mutation_rate"]
        self.p_usel = configs["p_usel"]
        self.p_bfa = configs["p_bfa"]
        self.chunk_size = configs["chunk_size"]
        self.hole_size = configs["hole_size"]
        self.nop_max_seq_size = configs["nop_max_seq_size"]
        self.mix = configs["mix"]
        self.population_size = options["population_size"]
        self.memory_size = options["memory_size"]
        self.test_size = options["test_size"]
        self.stride = max(options["stride"], self.memory_traits.word_size.value)
        self.instances = options["instances"]
        self.fitaddrs = 0
        self.nd_t = 0
        self.addresses = 0
        self.mcversi = None
        total_required_memory = (self.memory_size / self.chunk_size) * self.hole_size
        available_memory = 1 << self.memory_traits.address_size
        assert (
            total_required_memory <= available_memory
        ), "please increase address size to fit the chosen memory/chunk/hole sizes"

    def __call__(self):
        test = self.mcversi()
        self.tournament, self.generation, self.addresses = self.mcversi.test_info()
        return test

    def feedback(self, output):
        self.mcversi.feedback(output)
        self.fitaddrs = self.mcversi.fitaddrs_count()
        self.nd_t = self.mcversi.nd_t()
        return False

    def fields(self):
        return ["tournament", "generation", "addresses", "fitaddrs", "nd_t"]

    def field_values(self):
        return {
            "tournament": self.tournament,
            "generation": self.generation,
            "addresses": self.addresses,
            "fitaddrs": self.fitaddrs,
            "nd_t": self.nd_t,
        }

    def max_address_count(self):
        return self.memory_size // self.memory_traits.word_size.value

    def max_thread_size(self):
        return self.nop_max_seq_size * self.test_size

    def parameters(self):
        return {
            "test_size": self.test_size,
            "stride": self.stride,
            "instances": self.instances,
        }

    def trace_selection(self):
        return TraceSelection(
            {
                Operation.Type.LOAD: {Monitor.LOCAL_COMMIT},
                Operation.Type.STORE: {Monitor.LOCAL_COMPLETION},
            }
        )

    def set_seed(self, seed):
        self.mcversi = McVerSi(
            thread_count=self.thread_count,
            memory_size=self.memory_size,
            stride=self.stride,
            instances=self.instances,
            test_size=self.test_size,
            population_size=self.population_size,
            tournament_size=self.tournament_size,
            mates=self.mates,
            mutation_rate=self.mutation_rate,
            p_usel=self.p_usel,
            p_bfa=self.p_bfa,
            chunk_size=self.chunk_size,
            hole_size=self.hole_size,
            nop_max_seq_size=self.nop_max_seq_size,
            mix=McVerSi.OperationMix(self.mix),
            seed=seed,
        )

    def test_display(self):
        return "tournament: {}, generation: {}, s: {}".format(
            self.tournament, self.generation, self.addresses
        )

    def unique_id(self):
        return "{}.n{}.a{}.i{}".format(
            self.name,
            self.test_size,
            self.stride,
            self.instances,
        )

    @classmethod
    def define_options(cls, parser):
        cls.define_instances_option(parser)
        parser.add_argument(
            "-p",
            "--population-size",
            type=int,
            default=100,
            help="population size (tests)",
        )
        parser.add_argument(
            "-t",
            "--test-size",
            type=int,
            default=1000,
            help="test size (operations)",
        )
        parser.add_argument(
            "-m",
            "--memory-size",
            type=int,
            default=8192,
            help="usable memory size (bytes)",
        )
        parser.add_argument(
            "-s",
            "--stride",
            type=int,
            default=16,
            help="address stride (bytes)",
        )


__all__ = [Generator]
