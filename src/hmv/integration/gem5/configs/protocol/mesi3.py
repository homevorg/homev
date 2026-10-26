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

# Copyright (c) 2006-2007 The Regents of The University of Michigan
# Copyright (c) 2009,2015 Advanced Micro Devices, Inc.
# Copyright (c) 2013 Mark D. Hill and David A. Wood
# Copyright (c) 2020 ARM Limited
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
    LRURP,
    MemorySize,
    MessageBuffer,
    MESI_Three_Level_Directory_Controller,
    MESI_Three_Level_L0Cache_Controller,
    MESI_Three_Level_L1Cache_Controller,
    MESI_Three_Level_L2Cache_Controller,
    RubyCache,
    RubyDirectoryMemory,
    RubyPrefetcher,
    RubySequencer,
)

# internal imports
import fakefs


class DirectoryController(MESI_Three_Level_Directory_Controller):
    def __init__(self, ruby_system, version):
        super().__init__(
            version=version,
            directory=RubyDirectoryMemory(block_size=ruby_system.block_size_bytes),
            ruby_system=ruby_system,
            requestToDir=MessageBuffer(),
            responseToDir=MessageBuffer(),
            responseFromDir=MessageBuffer(),
            requestToMemory=MessageBuffer(),
            responseFromMemory=MessageBuffer(),
        )

    def connect_buffers(self, network):
        self.requestToDir.in_port = network.out_port
        self.responseToDir.in_port = network.out_port
        self.responseFromDir.out_port = network.in_port


class L1ICache(RubyCache):
    def __init__(self, params):
        super().__init__(
            dataAccessLatency=params["L1i"]["dataAccessLatency"],
            tagAccessLatency=params["L1i"]["tagAccessLatency"],
            size=params["L1i"]["size"],
            assoc=params["L1i"]["associativity"],
            start_index_bit=params["block_size_bits"],
            replacement_policy=LRURP(),
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
            replacement_policy=LRURP(),
            is_icache=False,
        )


class L2Cache(RubyCache):
    def __init__(self, params):
        super().__init__(
            dataAccessLatency=params["L2"]["dataAccessLatency"],
            tagAccessLatency=params["L2"]["tagAccessLatency"],
            size=params["L2"]["size"],
            assoc=params["L2"]["associativity"],
            start_index_bit=params["block_size_bits"],
        )


class L3Cache(RubyCache):
    def __init__(self, params):
        super().__init__(
            dataAccessLatency=params["L3"]["dataAccessLatency"],
            tagAccessLatency=params["L3"]["tagAccessLatency"],
            size=params["L3"]["size"],
            assoc=params["L3"]["associativity"],
            start_index_bit=params["block_size_bits"] + params["ll_bits"],
        )


class L1CacheController(MESI_Three_Level_L0Cache_Controller):
    def __init__(self, ruby_system, core, params):
        super().__init__(
            version=core.cpu_id,
            Icache=L1ICache(params),
            Dcache=L1DCache(params),
            prefetcher=RubyPrefetcher(block_size=ruby_system.block_size_bytes),
            enable_prefetch=params["enable_prefetch"],
            send_evictions=True,
            clk_domain=core.clk_domain,
            ruby_system=ruby_system,
            transitions_per_cycle=params["L1Controller"]["transitions_per_cycle"],
            request_latency=params["L1Controller"]["request_latency"],
            response_latency=params["L1Controller"]["response_latency"],
            prefetchQueue=MessageBuffer(),
            mandatoryQueue=MessageBuffer(),
            bufferToL1=MessageBuffer(ordered=True),
            bufferFromL1=MessageBuffer(ordered=True),
        )
        self.sequencer = RubySequencer(
            version=core.cpu_id,
            dcache=self.Dcache,
            clk_domain=core.clk_domain,
            ruby_system=ruby_system,
        )
        self.sequencer.connectCpuPorts(core)
        self.bufferFromL1.hmv_thread_monitor = core.hmv_thread_monitor

    def connect_buffers(self, network):
        pass


class L2CacheController(MESI_Three_Level_L1Cache_Controller):
    def __init__(self, ruby_system, l1_controller, params):
        super().__init__(
            version=l1_controller.version,
            cache=L2Cache(params),
            l2_select_num_bits=params["ll_bits"],
            cluster_id=l1_controller.version // params["core_count_per_llcache"],
            ruby_system=ruby_system,
            transitions_per_cycle=params["L2Controller"]["transitions_per_cycle"],
            l1_request_latency=params["L2Controller"]["request_latency"],
            l1_response_latency=params["L2Controller"]["response_latency"],
            to_l2_latency=params["L2Controller"]["to_latency"],
            requestToL2=MessageBuffer(),
            responseToL2=MessageBuffer(),
            unblockToL2=MessageBuffer(),
            requestFromL2=MessageBuffer(),
            responseFromL2=MessageBuffer(),
        )
        self.bufferFromL0 = l1_controller.bufferToL1
        self.bufferToL0 = l1_controller.bufferFromL1

    def connect_buffers(self, network):
        self.requestToL2.out_port = network.in_port
        self.responseToL2.out_port = network.in_port
        self.unblockToL2.out_port = network.in_port
        self.requestFromL2.in_port = network.out_port
        self.responseFromL2.in_port = network.out_port


class L3CacheController(MESI_Three_Level_L2Cache_Controller):
    def __init__(self, ruby_system, version, params):
        super().__init__(
            version=version,
            L2cache=L3Cache(params),
            cluster_id=version,
            ruby_system=ruby_system,
            transitions_per_cycle=params["L3Controller"]["transitions_per_cycle"],
            l2_request_latency=params["L3Controller"]["request_latency"],
            l2_response_latency=params["L3Controller"]["response_latency"],
            to_l1_latency=params["L3Controller"]["to_latency"],
            DirRequestFromL2Cache=MessageBuffer(),
            L1RequestFromL2Cache=MessageBuffer(),
            responseFromL2Cache=MessageBuffer(),
            unblockToL2Cache=MessageBuffer(),
            L1RequestToL2Cache=MessageBuffer(),
            responseToL2Cache=MessageBuffer(),
        )

    def connect_buffers(self, network):
        self.DirRequestFromL2Cache.out_port = network.in_port
        self.L1RequestFromL2Cache.out_port = network.in_port
        self.responseFromL2Cache.out_port = network.in_port
        self.unblockToL2Cache.in_port = network.out_port
        self.L1RequestToL2Cache.in_port = network.out_port
        self.responseToL2Cache.in_port = network.out_port


def instantiate(ruby_system, cores, params):
    _define_dynamic_params(cores, params)
    _register_with_fakefs(cores, params)
    l1_controllers = [L1CacheController(ruby_system, c, params) for c in cores]
    ruby_system.controllers = (
        l1_controllers
        + [L2CacheController(ruby_system, c, params) for c in l1_controllers]
        + [
            L3CacheController(ruby_system, i, params)
            for i in range(params["num_llcaches"])
        ]
    )


def _define_dynamic_params(cores, params):
    params["number_of_virtual_networks"] = 3
    params["core_count_per_llcache"] = len(cores) // params["num_llcaches"]
    params["ll_bits"] = 0


def _register_with_fakefs(cores, params):
    for i in range(len(cores)):
        fakefs.write_cache_params(2, 2, [i], params)
    for i in range(params["num_llcaches"]):
        core_range_begin = i * params["core_count_per_llcache"]
        core_range_end = (i + 1) * params["core_count_per_llcache"]
        fakefs.write_cache_info(
            3,
            3,
            str(MemorySize(params["L3"]["size"])),
            params["L3"]["associativity"],
            list(range(core_range_begin, core_range_end)),
            params["block_size"],
        )
