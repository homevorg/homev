# Copyright (C) 2019-2026 Marleson Graf

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

from m5.defines import buildEnv
from m5.params import *
from m5.objects.ClockedObject import ClockedObject

_has_mesi_two = "RUBY_PROTOCOL_MESI_Two_Level" in buildEnv
_has_mesi_three = "RUBY_PROTOCOL_MESI_Three_Level" in buildEnv
_has_moesi_cmp = "RUBY_PROTOCOL_MOESI_CMP_directory" in buildEnv
_faults = ["NONE"]

if _has_mesi_two:
    # Faults for L1 controllers of MESI_Two_Level protocol
    _faults += [
        # "MESI2_L1_F01",
    ]
if _has_mesi_two or _has_mesi_three:
    # Faults for L2 controllers of MESI_Two_Level/MESI_Three_Level protocol
    _faults += [
        "MESI2_L2_F01",
    ]
if _has_mesi_three:
    # Faults for L0 controllers of MESI_Three_Level protocol
    _faults += [
        # "MESI3_L0_F01",
    ]
    # Faults for L1 controllers of MESI_Three_Level protocol
    _faults += [
        "MESI3_L1_F01",
        "MESI3_L1_F02",
    ]
if _has_moesi_cmp:
    # Faults for L1 controllers of MOESI_CMP_directory protocol
    _faults += [
        "MOESI_L1_F01",
        "MOESI_L1_F02",
        "MOESI_L1_F03",
        "MOESI_L1_F04",
    ]
    # Faults for L2 controllers of MOESI_CMP_directory protocol
    _faults += [
        "MOESI_L2_F01",
        "MOESI_L2_F02",
        "MOESI_L2_F03",
        "MOESI_L2_F04",
        "MOESI_L2_F05",
    ]


class HMVWordSize(ScopedEnum):
    # Keep this list in sync with hmv::WordSize
    vals = ["DWORD", "QWORD"]
    map = {"DWORD": 4, "QWORD": 8}


class HMVChecker(ScopedEnum):
    # Keep this list in sync with hmv::checker::Type
    vals = ["HOCH", "EVCH", "MRSB", "SRSB", "LCHECK"]


class HMVCoverageMetric(ScopedEnum):
    # Keep this list in sync with hmv::coverage::Type
    vals = ["FSM_STRUCTURAL", "FSM_PER_INSTANCE"]


class HMVISA(ScopedEnum):
    # Keep this list in sync with hmv::ISA
    vals = ["ARM", "RISCV"]


class HMVFault(ScopedEnum):
    vals = _faults


class HMVController(ClockedObject):
    type = "HMVController"
    cxx_class = "gem5::HMVController"
    cxx_header = "hmv/integration/gem5/hmv_controller.hh"

    thread_count = Param.UInt16("Number of threads in the system.")
    coverage_metrics = VectorParam.HMVCoverageMetric([], "Coverage metrics to be used.")
    coverage_totals = VectorParam.UInt64([], "Total coverage values for each metric.")
    checkers = VectorParam.HMVChecker([], "Checkers to be used.")
    address_size = Param.UInt16("Address size (in bits).")
    word_size = Param.HMVWordSize("Word size.")
    physical_memory_size = Param.UInt64("Size of the physical memory (in bytes).")
    shared_memory_region = Param.UInt64(
        "Virtual address pointing to start of shared region."
    )
    max_thread_size = Param.UInt64("Maximum thread size (in operations).")
    generator_pid = Param.Int32("generator pid (to synchronize with test loader).")
    fault = Param.HMVFault(_faults[0], "Fault to be injected.")
    warmup_instances = Param.UInt32(0, "number of warmup execution instances.")
    details = Param.Bool(False, "Enable detailed HMV output.")
    architecture = Param.HMVISA("Target Instruction Set Architecture.")
    trace_read_sel = VectorParam.UInt8("Selection of monitors to trace read events.")
    trace_write_sel = VectorParam.UInt8("Selection of monitors to trace write events.")
    trace_fence_sel = VectorParam.UInt8("Selection of monitors to trace fence events.")
