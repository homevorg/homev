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

# Copyright (c) 2012, 2017-2018, 2021 ARM Limited
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
# Copyright (c) 2006-2007 The Regents of The University of Michigan
# Copyright (c) 2009 Advanced Micro Devices, Inc.
# All rights reserved.
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
import math

# gem5 imports
from m5.objects import (
    AddrRange,
    DDR3_1600_8x8,
    DDR3_2133_8x8,
    DDR4_2400_8x8,
    DDR5_4400_4x8,
    DDR5_6400_4x8,
    DDR5_8400_4x8,
    DRAMInterface,
    ExternalSlave,
    IOXBar,
    MemCtrl,
    SimpleMemory,
)

_supported_dram_types = {
    "DDR3_1600_8x8": DDR3_1600_8x8,
    "DDR3_2133_8x8": DDR3_2133_8x8,
    "DDR4_2400_8x8": DDR4_2400_8x8,
    "DDR5_4400_4x8": DDR5_4400_4x8,
    "DDR5_6400_4x8": DDR5_6400_4x8,
    "DDR5_8400_4x8": DDR5_8400_4x8,
}


def _create_memory_controller(mem_range, index, params):
    assert (
        params["System"]["memory_type"] in _supported_dram_types.keys()
    ), "unsupported memory type"
    DRAMClass = _supported_dram_types[params["System"]["memory_type"]]
    if DRAMClass.addr_mapping.value == "RoRaBaChCo":
        rowbuffer_size = (
            DRAMClass.device_rowbuffer_size.value * DRAMClass.devices_per_rank.value
        )
        intlv_low_bit = int(math.log(rowbuffer_size, 2))
    else:
        intlv_low_bit = int(math.log(params["Ruby"]["block_size"], 2))
    intlv_bits = int(math.log(params["Ruby"]["num_dirs"], 2))
    return MemCtrl(
        dram=DRAMClass(
            range=AddrRange(
                mem_range.start,
                size=mem_range.size(),
                intlvHighBit=intlv_low_bit + intlv_bits - 1,
                xorHighBit=0,  # or 20 + intlv_bits - 1,
                intlvBits=intlv_bits,
                intlvMatch=index,
            ),
        )
    )


def instantiate(system, params):
    mem_ctrls = []
    crossbars = []
    for index, directory in enumerate(system.ruby.directory_controllers):
        crossbar = None
        if len(system.mem_ranges) > 1:
            crossbar = IOXBar()
            crossbars.append(crossbar)
            directory.memory_out_port = crossbar.cpu_side_ports
        dir_ranges = []
        for mem_range in system.mem_ranges:
            mem_ctrl = _create_memory_controller(mem_range, index, params)
            dir_ranges.append(mem_ctrl.dram.range)
            mem_ctrls.append(mem_ctrl)
            if crossbar != None:
                mem_ctrl.port = crossbar.mem_side_ports
            else:
                mem_ctrl.port = directory.memory_out_port
        directory.addr_ranges = dir_ranges
    system.mem_ctrls = mem_ctrls
    if len(crossbars) > 0:
        system.ruby.crossbars = crossbars
