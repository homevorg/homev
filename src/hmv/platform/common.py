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
import subprocess

from hmv import ISA, WordSize
from hmv.platform import Output, Traits


class BaseDispatcher:
    def __init__(
        self,
        install_dir,
        memory_traits,
        architecture,
        core_count,
        warmup_instances,
        checkers,
        metrics,
        details,
    ):
        self.install_dir = os.path.abspath(install_dir)
        self.memory_traits = memory_traits
        self.architecture = architecture
        self.core_count = core_count
        self.warmup_instances = warmup_instances
        self.checkers = checkers
        self.metrics = metrics
        self.details = details
        self.test_flags = [
            "-std=gnu++20",
            "-static",
            "-O2",
            "-fno-rtti",
            "-fno-exceptions",
            "-fno-toplevel-reorder",
            "-fno-reorder-blocks",
            "-fno-delayed-branch",
            "-fno-threadsafe-statics",
            "-fno-tree-vectorize",
            "-flto",
            "-fconserve-stack",
            "-fvisibility=hidden",
            "-ffunction-sections",
            "-fdata-sections",
            "-Wl,--gc-sections",
            "-DHTS_THREAD_COUNT={}".format(self.core_count),
        ]
        if self.memory_traits.address_size > 32:
            self.test_flags.append("-DHTS_USE_QUAD_WORD_INDEX")
        if self.memory_traits.word_size == WordSize.QWORD:
            self.test_flags.append("-DHTS_USE_QUAD_WORD_VALUE")

    def compile(self, sources, executable, include_dir, extra_flags=[]):
        process = subprocess.run(
            [
                self.test_compiler,
                *sources,
                "-o",
                executable,
                "-I{}".format(include_dir),
                *self.test_flags,
                "-DHTS_MAX_ADDRESS_COUNT={}".format(self.max_address_count),
                *extra_flags,
            ],
            stdout=subprocess.DEVNULL,
        )
        assert process.returncode == 0, "failed to compile test"

    def dispatch(self, test):
        assert len(test.threads) == self.core_count, "incompatible test thread count"
        test.write(Traits.test_file)
        return self.execute(test), Output.read(Traits.output_file)

    def set_max_address_count(self, count):
        self.max_address_count = count

    def set_trace_selection(self, trace_selection):
        self.trace_selection = trace_selection

    def deploy(self, seed):
        assert False, "deploy() not overridden!"

    def execute(self, test):
        assert False, "execute() not overridden!"

    def destroy(self):
        assert False, "destroy() not overridden!"

    def set_max_thread_size(self, size):
        assert False, "set_max_thread_size() not overridden!"

    def get_last_level_cache_traits(self):
        assert False, "get_last_level_cache_traits() not overridden!"

    def parameters(self):
        assert False, "parameters() not overridden!"

    @classmethod
    def define_options(cls, parser):
        assert False, "define_options() not overridden!"


class BaseBuilder:
    def __init__(self, configs, install_dir):
        self.configs = configs
        self.install_dir = install_dir

    def __call__(self, job_count):
        assert False, "__call__() not overridden!"

    @classmethod
    def define_options(cls, parser):
        pass
