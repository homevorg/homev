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
    MessageBuffer,
    MOESI_CMP_directory_Directory_Controller,
    MOESI_CMP_directory_L1Cache_Controller,
    MOESI_CMP_directory_L2Cache_Controller,
    RubyCache,
    RubyDirectoryMemory,
    RubySequencer,
)

# internal imports
import fakefs


class DirectoryController(MOESI_CMP_directory_Directory_Controller):
    def __init__(self, ruby_system, version):
        super().__init__(
            version=version,
            directory=RubyDirectoryMemory(block_size=ruby_system.block_size_bytes),
            ruby_system=ruby_system,
            requestToDir=MessageBuffer(),
            responseToDir=MessageBuffer(),
            responseFromDir=MessageBuffer(),
            forwardFromDir=MessageBuffer(),
            requestToMemory=MessageBuffer(),
            responseFromMemory=MessageBuffer(),
            triggerQueue=MessageBuffer(ordered=True),
        )

    def connect_buffers(self, network):
        self.requestToDir.in_port = network.out_port
        self.responseToDir.in_port = network.out_port
        self.responseFromDir.out_port = network.in_port
        self.forwardFromDir.out_port = network.in_port


class L1ICache(RubyCache):
    def __init__(self, params):
        super().__init__(
            dataAccessLatency=params["L1i"]["dataAccessLatency"],
            tagAccessLatency=params["L1i"]["tagAccessLatency"],
            size=params["L1i"]["size"],
            assoc=params["L1i"]["associativity"],
            start_index_bit=params["block_size_bits"],
            is_icache=True,
        )


class L1DCache(RubyCache):
    def __init__(self, params):
        super().__init__(
            dataAccessLatency=params["L1d"]["dataAccessLatency"],
            tagAccessLatency=params["L1d"]["tagAccessLatency"],
            size=params["L1d"]["size"],
            assoc=params["L1d"]["associativity"],
            start_index_bit=params["block_size_bits"],
            is_icache=False,
        )


class L2Cache(RubyCache):
    def __init__(self, params):
        super().__init__(
            dataAccessLatency=params["L2"]["dataAccessLatency"],
            tagAccessLatency=params["L2"]["tagAccessLatency"],
            size=params["L2"]["size"],
            assoc=params["L2"]["associativity"],
            start_index_bit=params["block_size_bits"] + params["ll_bits"],
        )


class L1CacheController(MOESI_CMP_directory_L1Cache_Controller):
    def __init__(self, ruby_system, core, params):
        super().__init__(
            version=core.cpu_id,
            L1Icache=L1ICache(params),
            L1Dcache=L1DCache(params),
            send_evictions=True,
            clk_domain=core.clk_domain,
            ruby_system=ruby_system,
            transitions_per_cycle=params["L1Controller"]["transitions_per_cycle"],
            request_latency=params["L1Controller"]["request_latency"],
            response_latency=params["L1Controller"]["response_latency"],
            mandatoryQueue=MessageBuffer(),
            requestFromL1Cache=MessageBuffer(),
            responseFromL1Cache=MessageBuffer(),
            requestToL1Cache=MessageBuffer(),
            responseToL1Cache=MessageBuffer(),
            triggerQueue=MessageBuffer(ordered=True),
        )
        self.sequencer = RubySequencer(
            version=core.cpu_id,
            dcache=self.L1Dcache,
            clk_domain=core.clk_domain,
            ruby_system=ruby_system,
        )
        self.sequencer.connectCpuPorts(core)
        self.responseToL1Cache.hmv_thread_monitor = core.hmv_thread_monitor

    def connect_buffers(self, network):
        self.requestFromL1Cache.out_port = network.in_port
        self.responseFromL1Cache.out_port = network.in_port
        self.requestToL1Cache.in_port = network.out_port
        self.responseToL1Cache.in_port = network.out_port


class L2CacheController(MOESI_CMP_directory_L2Cache_Controller):
    def __init__(self, ruby_system, version, params):
        super().__init__(
            version=version,
            L2cache=L2Cache(params),
            addr_ranges=[
                AddrRange(
                    mem_range.start,
                    size=mem_range.size(),
                    intlvHighBit=params["block_size_bits"] + params["ll_bits"] - 1,
                    intlvBits=params["ll_bits"],
                    intlvMatch=version,
                )
                for mem_range in params["memory_ranges"]
            ],
            ruby_system=ruby_system,
            transitions_per_cycle=params["L2Controller"]["transitions_per_cycle"],
            request_latency=params["L2Controller"]["request_latency"],
            response_latency=params["L2Controller"]["response_latency"],
            GlobalRequestFromL2Cache=MessageBuffer(),
            L1RequestFromL2Cache=MessageBuffer(),
            responseFromL2Cache=MessageBuffer(),
            GlobalRequestToL2Cache=MessageBuffer(),
            L1RequestToL2Cache=MessageBuffer(),
            responseToL2Cache=MessageBuffer(),
            triggerQueue=MessageBuffer(ordered=True),
        )

    def connect_buffers(self, network):
        self.GlobalRequestFromL2Cache.out_port = network.in_port
        self.L1RequestFromL2Cache.out_port = network.in_port
        self.responseFromL2Cache.out_port = network.in_port
        self.GlobalRequestToL2Cache.in_port = network.out_port
        self.L1RequestToL2Cache.in_port = network.out_port
        self.responseToL2Cache.in_port = network.out_port


def instantiate(ruby_system, cores, params):
    _define_dynamic_params(params)
    _register_with_fakefs(cores, params)
    ruby_system.controllers = [
        L1CacheController(ruby_system, c, params) for c in cores
    ] + [L2CacheController(ruby_system, i, params) for i in range(params["num_llcaches"])]


def _define_dynamic_params(params):
    params["number_of_virtual_networks"] = 3
    params["ll_bits"] = int(math.log(params["num_llcaches"], 2))


def _register_with_fakefs(cores, params):
    core_count_per_llcache = len(cores) // params["num_llcaches"]
    for i in range(params["num_llcaches"]):
        core_range_begin = i * core_count_per_llcache
        core_range_end = (i + 1) * core_count_per_llcache
        fakefs.write_cache_info(
            2,
            2,
            params["L2"]["size"],
            params["L2"]["associativity"],
            list(range(core_range_begin, core_range_end)),
            params["block_size"],
        )
