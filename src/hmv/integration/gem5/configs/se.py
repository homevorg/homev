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
import argparse
import os

# gem5 imports
import m5
from m5.defines import buildEnv
from m5.objects import (
    AddrRange,
    O3CPU,
    Process,
    Root,
    SEWorkload,
    SrcClockDomain,
    System,
    VoltageDomain,
)

# internal imports
import cache
import cpu
import fakefs
import memory
import hmv


def create_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument("-w", "--workload", required=True, help="workload to run")
    return parser


def instantiate(args):
    params, controller = hmv.instantiate(args)
    params["Ruby"]["memory_ranges"] = [
        AddrRange(start=0, size=params["System"]["memory_size"])
    ]
    cores, memory_mode = cpu.instantiate(params["CPU"])
    system = System(
        cpu=cores,
        mem_mode=memory_mode,
        mem_ranges=params["Ruby"]["memory_ranges"],
        cache_line_size=params["Ruby"]["block_size"],
        multi_thread=False,
        clk_domain=SrcClockDomain(
            clock=params["System"]["clock"],
            voltage_domain=VoltageDomain(voltage=params["System"]["voltage"]),
        ),
        hmv_controller=controller,
    )
    hmv.connect(system, args)
    _setup_workload(system, args.workload)
    system.redirect_paths = fakefs.instantiate(params)
    cache.instantiate(system, params["Ruby"])
    memory.instantiate(system, params)
    return params, system


def _setup_workload(system, workload):
    process = Process(
        pid=100, gid=os.getgid(), cwd=os.getcwd(), cmd=[workload], executable=workload
    )
    system.workload = SEWorkload.init_compatible(process.executable)
    system.workload.wait_for_remote_gdb = False
    system.workload.remote_gdb_port = 0
    for core in system.cpu:
        core.workload = process
        core.createThreads()


def main():
    args = hmv.define_options(create_parser()).parse_args()
    # Create root and system
    params, system = instantiate(args)
    root = Root(system=system, full_system=False)
    # Instantiate C++ objects
    m5.instantiate()
    # Run simulation
    event = m5.simulate()
    # Report end of simulation
    print("Exiting @ tick %i because %s" % (m5.curTick(), event.getCause()))
    # Report error code in case of error
    if event.getCode() != 0:
        print("Simulated exit code not 0! Exit code is", event.getCode())


if __name__ == "__m5_main__":
    main()
