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

# gem5 imports
from m5.objects import ArmO3CPU, RiscvO3CPU, SrcClockDomain, VoltageDomain

_types = {"O3": {"ARM": ArmO3CPU, "RISCV": RiscvO3CPU}}
_attributes = [
    "LQEntries",
    "SQEntries",
    "LSQDepCheckShift",
    "LFSTSize",
    "SSITSize",
    "decodeToFetchDelay",
    "renameToFetchDelay",
    "iewToFetchDelay",
    "commitToFetchDelay",
    "renameToDecodeDelay",
    "iewToDecodeDelay",
    "commitToDecodeDelay",
    "iewToRenameDelay",
    "commitToRenameDelay",
    "commitToIEWDelay",
    "fetchWidth",
    "fetchBufferSize",
    "fetchToDecodeDelay",
    "decodeWidth",
    "decodeToRenameDelay",
    "renameWidth",
    "renameToIEWDelay",
    "issueToExecuteDelay",
    "dispatchWidth",
    "issueWidth",
    "wbWidth",
    "iewToCommitDelay",
    "renameToROBDelay",
    "commitWidth",
    "squashWidth",
    "trapLatency",
    "backComSize",
    "forwardComSize",
    "numPhysIntRegs",
    "numPhysFloatRegs",
    "numPhysVecRegs",
    "numIQEntries",
    "numROBEntries",
]

def instantiate(params):
    CPUClass = _get_cpu_class(params)
    cores = [_create_core(CPUClass, i, params) for i in range(params["core_count"])]
    return cores, CPUClass.memory_mode()


def _get_cpu_class(params):
    CPUClass = _types[params["type"]][params["architecture"]]
    # Make sure each CPU only has 1 thread (no SMT)
    CPUClass.numThreads = 1
    for attribute in _attributes:
        if attribute in params and hasattr(CPUClass, attribute):
            setattr(CPUClass, attribute, params[attribute])
    return CPUClass


def _create_core(CPUClass, index, params):
    core = CPUClass(
        cpu_id=index,
        clk_domain=SrcClockDomain(
            clock=params["clock"],
            voltage_domain=VoltageDomain(voltage=params["voltage"]),
        ),
    )
    # Create the interrupt controller
    core.createInterruptController()
    return core
