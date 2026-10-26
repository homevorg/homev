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

import os
import tomlkit

from .common import BaseGenerator
from hmv import Operation, Load, Fence, Nop, Store, Test


class Generator(BaseGenerator):
    name = "UDT"
    description = "User-Defined Tests Generator"
    configuration_key = "UDT"

    def __init__(
        self, memory_traits, cache_traits, thread_count, metrics, configs, **options
    ):
        super().__init__(memory_traits, cache_traits, thread_count, metrics)
        self.instances = options["instances"]
        self.tests_file = options["tests_file"]
        self.current_test = 0
        self._max_thread_size = 0
        self._parse()

    def __call__(self):
        if self.current_test >= len(self.tests):
            return None
        test = self.tests[self.current_test]
        self.n = sum([len(thread) for thread in test.threads])
        self.s = len(test.addresses)
        self.current_test += 1
        return test

    def feedback(self, output):
        return False

    def fields(self):
        return ["n", "s"]

    def field_values(self):
        return {"n": self.n, "s": self.s}

    def max_address_count(self):
        return self._max_address_count

    def max_thread_size(self):
        return self._max_thread_size

    def parameters(self):
        return {
            "tests_file": self.tests_file,
            "test_count": len(self.tests),
            "max_address_count": self._max_address_count,
            "max_thread_size": self._max_thread_size,
            "instances": self.instances,
        }

    def set_seed(self, seed):
        self.current_test = 0

    def test_display(self):
        return "n = {}, s = {}".format(self.n, self.s)

    def unique_id(self):
        return "{}.{}.c{}.a{}.t{}.i{}".format(
            self.name,
            self.id,
            len(self.tests),
            self._max_address_count,
            self._max_thread_size,
            self.instances,
        )

    def _parse(self):
        with open(self.tests_file, "rb") as file:
            document = tomlkit.load(file)
        self.id = document["id"]
        self.tests = []
        for test in document["tests"]:
            threads = []
            for udt_thread in test["threads"]:
                thread = []
                thread_size = len(udt_thread)
                for inst in udt_thread:
                    if inst["type"] == Operation.Type.LOAD.name:
                        thread.append(Load(inst["address"]))
                    if inst["type"] == Operation.Type.STORE.name:
                        thread.append(Store(inst["address"], inst["value"]))
                    if inst["type"] == Operation.Type.FENCE.name:
                        thread.append(Fence())
                    if inst["type"] == Operation.Type.NOP.name:
                        thread.append(Nop(inst["amount"]))
                        thread_size += inst["amount"] - 1
                if thread_size > self._max_thread_size:
                    self._max_thread_size = thread_size
                threads.append(thread)
            self.tests.append(Test(test["addresses"], threads, self.instances))
        self._max_address_count = max([len(test.addresses) for test in self.tests])

    @classmethod
    def define_options(cls, parser):
        cls.define_instances_option(parser)
        parser.add_argument(
            "-t",
            "--tests-file",
            required=True,
            help="TOML file containing the user-defined tests",
        )


__all__ = [Generator]
