# Copyright (C) 2024-2026 Marleson Graf

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

# System imports
from itertools import chain
import os
import tomllib

# gem5 imports
from m5 import options as global_options
from m5.objects import (
    HMVChecker,
    HMVCoverageMetric,
    HMVController,
    HMVThreadMonitor,
    HMVFault,
    HMVISA,
    HMVWordSize,
    SrcClockDomain,
    VoltageDomain,
)
from m5.defines import buildEnv
from m5.util.convert import toMemorySize

_monitors = {
    "LOCAL_COMMIT": 0,
    "REMOTE_COMMIT": 1,
    "LOCAL_COMPLETION": 2,
    "REMOTE_COMPLETION": 3,
    "SQUASH": 4,
    "AVAILABILITY": 5,
    "MEMORY": 6,
    "ENTER_TIME": 7,
    "COMMIT_TIME": 8,
}
_mem_sizes = {
    0x40000000: "1GiB",
    0x80000000: "2GiB",
    0x100000000: "4GiB",
    0x200000000: "8GiB",
    0x400000000: "16GiB",
}


def define_options(parser):
    parser.add_argument(
        "-p",
        "--parameters-file",
        required=True,
        help="parameters file (TOML)",
    )
    parser.add_argument(
        "-g",
        "--generator-pid",
        type=int,
        default=-1,
        help="generator pid (to synchronize with test loader)",
    )
    parser.add_argument(
        "-d",
        "--details",
        action="store_true",
        default=False,
        help="enable detailed HMV output",
    )
    return parser


def instantiate(args):
    with open(args.parameters_file, "rb") as parameters_file:
        params = tomllib.load(parameters_file)
    _define_memory_params(params)
    trace_read_sel, trace_write_sel, trace_fence_sel = _get_trace_selections(params)
    core_count = params["CPU"]["core_count"]
    return params, HMVController(
        thread_count=core_count,
        coverage_metrics=params["metrics"],
        coverage_totals=[
            _coverage_total(m, core_count, params["Ruby"]) for m in params["metrics"]
        ],
        checkers=params["checkers"],
        address_size=params["address_size"],
        word_size=params["word_size"],
        physical_memory_size=params["physical_memory_size"],
        shared_memory_region=params["shared_memory_region"],
        max_thread_size=params["max_thread_size"],
        generator_pid=args.generator_pid,
        fault=params["fault"],
        warmup_instances=params["warmup_instances"],
        details=args.details,
        architecture=params["CPU"]["architecture"],
        trace_read_sel=trace_read_sel,
        trace_write_sel=trace_write_sel,
        trace_fence_sel=trace_fence_sel,
        clk_domain=SrcClockDomain(
            clock=params["CPU"]["clock"],
            voltage_domain=VoltageDomain(voltage=params["CPU"]["voltage"]),
        ),
    )


def connect(system, args):
    for i, cpu in enumerate(system.cpu):
        cpu.hmv_thread_monitor = HMVThreadMonitor(
            controller=system.hmv_controller,
            tid=i,
        )


def _coverage_total(metric, core_count, params):
    controllers = ["L1Controller", "L2Controller"]
    if params["protocol"] == "MESI_Three_Level":
        controllers.append("L3Controller")
    if metric == "FSM_STRUCTURAL":
        return sum([params[c]["transition_count"] for c in controllers if c in params])
    elif metric == "FSM_PER_INSTANCE":
        return params[controllers[-1]]["transition_count"] + sum(
            [params[c]["transition_count"] * core_count for c in controllers[:-1]]
        )
    return 1


def _define_memory_params(params):
    assert params["address_size"] <= 33, "address size can't be bigger than 33"
    safety_margin = 0x40000000
    shared_size = 1 << params["address_size"]
    minimum_required_size = shared_size + safety_margin
    size = next((size for size in _mem_sizes.keys() if size > minimum_required_size))
    params["physical_memory_size"] = size
    params["System"]["memory_size"] = _mem_sizes[size]


def _get_trace_selections(params):
    fn = lambda monitors: list({_monitors[m] for m in monitors})
    trace_read_sel = []
    trace_write_sel = []
    trace_fence_sel = []
    if "trace_selection" in params:
        if "LOAD" in params["trace_selection"]:
            trace_read_sel = fn(params["trace_selection"]["LOAD"])
        if "STORE" in params["trace_selection"]:
            trace_write_sel = fn(params["trace_selection"]["STORE"])
        if "FENCE" in params["trace_selection"]:
            trace_fence_sel = fn(params["trace_selection"]["FENCE"])
    return trace_read_sel, trace_write_sel, trace_fence_sel
