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
import shutil
import time

from hmv import Range, TraceSelection


class BaseGenerator:

    def __init__(self, memory_traits, cache_traits, thread_count, metrics):
        self.memory_traits = memory_traits
        self.cache_traits = cache_traits
        self.thread_count = thread_count
        self.metrics = metrics

    def trace_selection(self):
        return TraceSelection()

    def __call__(self):
        assert False, "__call__() not overridden!"

    def feedback(self, output):
        assert False, "__call__() not overridden!"

    def fields(self):
        assert False, "fields() not overridden!"

    def field_values(self):
        assert False, "field_values() not overridden!"

    def max_address_count(self):
        assert False, "max_address_count() not overridden!"

    def max_thread_size(self):
        assert False, "max_thread_size() not overridden!"

    def parameters(self):
        assert False, "parameters() not overridden!"

    def set_seed(self, seed):
        assert False, "set_seed() not overridden!"

    def unique_id(self):
        assert False, "unique_id() not overridden!"

    def test_display(self):
        assert False, "test_display() not overridden!"

    @classmethod
    def define_options(cls, parser):
        assert False, "define_options() not overridden!"

    @classmethod
    def define_n_range_options(cls, parser):
        parser.add_argument(
            "-n", "--nmin", required=True, type=int, help="minimum test size"
        )
        parser.add_argument(
            "-N",
            "--nmax",
            type=int,
            default=0,
            help="maximum test size (defaults to nmin)",
        )

    @classmethod
    def define_s_range_options(cls, parser):
        parser.add_argument(
            "-s", "--smin", required=True, type=int, help="minimum address count"
        )
        parser.add_argument(
            "-S",
            "--smax",
            type=int,
            default=0,
            help="maximum address count (defaults to smin)",
        )

    @classmethod
    def define_instances_option(cls, parser):
        parser.add_argument(
            "-i",
            "--instances",
            "--execution-instances",
            required=True,
            type=int,
            help="number of execution instances",
        )

    @classmethod
    def make_range(cls, rmin, rmax):
        return Range(rmin, max(rmin, rmax))
