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

# Copyright (c) 2019 ARM Limited
# All rights reserved.
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Copyright (c) 2015 Advanced Micro Devices, Inc.
# All rights reserved
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# System imports
import os
import shutil

# gem5 imports
import m5
from m5.objects import RedirectPath
from m5.util.convert import kibi, mega, toFrequency, toMemorySize

_root_dir = os.path.join(m5.options.outdir, "fs")
_proc_dir = os.path.join(_root_dir, "proc")
_sys_dir = os.path.join(_root_dir, "sys")
_tmp_dir = os.path.join(_root_dir, "tmp")
_cpu_dir = os.path.join(_sys_dir, "devices", "system", "cpu")
_cpuinfo_str = """processor	: {index}
vendor_id	: Generic
cpu family	: 0
model		: 0
model name	: Generic
stepping	: 0
microcode	: 0x0000000
cpu MHz		: {clock:0.3f}
cache size	: {cache_size} KB
physical id	: 0
siblings	: {core_count}
core id		: {index}
cpu cores	: {core_count}
fpu		: yes
fpu_exception	: yes
cpuid level	: 1
wp		: yes
flags		: fpu
cache_alignment	: {cache_alignment}
address sizes	: {address_size} bits physical, {address_size} bits virtual
"""
_core_dir = lambda i: os.path.join(_cpu_dir, f"cpu{i}")
_to_hex = lambda core_ids: f"{sum([1 << i for i in core_ids]):0x}"


def _write_to(path, filename, contents):
    with open(os.path.join(path, filename), "a") as file:
        file.write(str(contents) + "\n")


def _init_directories():
    if os.path.isdir(_root_dir):
        shutil.rmtree(_root_dir)
    os.makedirs(_root_dir)
    os.makedirs(_proc_dir)
    os.makedirs(_sys_dir)
    os.makedirs(_tmp_dir)
    os.makedirs(_cpu_dir)


def _get_cache_size(params):
    if params["protocol"] == "MESI_Three_Level":
        return toMemorySize(params["L2"]["size"])
    return toMemorySize(params["L1i"]["size"]) + toMemorySize(params["L1d"]["size"])


def instantiate(params):
    _init_directories()
    _write_to(_proc_dir, "stat", "cpu 0 0 0 0 0 0 0 0 0 0")
    for index in range(params["CPU"]["core_count"]):
        top_dir = os.path.join(_core_dir(index), "topology")
        os.makedirs(top_dir)
        _write_to(
            _proc_dir,
            "cpuinfo",
            _cpuinfo_str.format(
                index=index,
                clock=toFrequency(params["CPU"]["clock"]) / mega,
                cache_size=_get_cache_size(params["Ruby"]),
                core_count=params["CPU"]["core_count"],
                cache_alignment=params["Ruby"]["block_size"],
                address_size=params["Ruby"]["address_size_bits"],
            ),
        )
        _write_to(_proc_dir, "stat", f"cpu{index} 0 0 0 0 0 0 0 0 0 0")
        _write_to(_core_dir(index), "online", "1")
        _write_to(top_dir, "physical_package_id", 0)
        _write_to(top_dir, "core_siblings", _to_hex(range(params["CPU"]["core_count"])))
        _write_to(top_dir, "core_id", index)
        _write_to(top_dir, "thread_siblings", 0)
        write_cache_params(0, 1, [index], params["Ruby"], "Instruction", "i")
        write_cache_params(1, 1, [index], params["Ruby"], "Data", "d")
    _write_to(_cpu_dir, "online", f"0-{params["CPU"]["core_count"] - 1}")
    _write_to(_cpu_dir, "possible", f"0-{params["CPU"]["core_count"] - 1}")
    return [
        RedirectPath(app_path="/proc", host_paths=[_proc_dir]),
        RedirectPath(app_path="/sys", host_paths=[_sys_dir]),
        RedirectPath(app_path="/tmp", host_paths=[_tmp_dir]),
    ]


def write_cache_info(index, level, size, assoc, cores, block_size, _type="Unified"):
    size = toMemorySize(size)
    number_of_sets = size // (assoc * block_size)
    shared_cpu_list = ",".join(str(core) for core in cores)
    for i in cores:
        index_dir = os.path.join(_core_dir(i), "cache", f"index{index}")
        os.makedirs(index_dir)
        _write_to(index_dir, "level", level)
        _write_to(index_dir, "type", _type)
        _write_to(index_dir, "size", f"{size // kibi}K")
        _write_to(index_dir, "coherency_line_size", block_size)
        _write_to(index_dir, "number_of_sets", number_of_sets)
        _write_to(index_dir, "ways_of_associativity", assoc)
        _write_to(index_dir, "physical_line_partition", 1)
        _write_to(index_dir, "shared_cpu_map", _to_hex(cores))
        _write_to(index_dir, "shared_cpu_list", shared_cpu_list)


def write_cache_params(index, level, cores, params, _type="Unified", _suffix=""):
    write_cache_info(
        index,
        level,
        params["L{}{}".format(level, _suffix)]["size"],
        params["L{}{}".format(level, _suffix)]["associativity"],
        cores,
        params["block_size"],
        _type,
    )
