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
import math

# gem5 imports
from m5.defines import buildEnv
from m5.objects import (
    RubyPortProxy,
    RubySystem,
    SrcClockDomain,
)

# internal imports
import network
from protocol import mesi2, mesi3, moesi

_protocols = {
    "MESI_Two_Level": mesi2,
    "MESI_Three_Level": mesi3,
    "MOESI_CMP_directory": moesi,
}


def instantiate(system, params):
    params["block_size_bits"] = int(math.log(params["block_size"], 2))
    system.ruby = RubySystem(
        block_size_bytes=params["block_size"],
        memory_size_bits=params["address_size_bits"],
        num_of_sequencers=len(system.cpu),
        clk_domain=SrcClockDomain(
            clock=params["clock"], voltage_domain=system.clk_domain.voltage_domain
        ),
    )
    _instantiate_controllers(system, params)
    network.instantiate(system.ruby, params)
    system.ruby.number_of_virtual_networks = params["number_of_virtual_networks"]
    system.sys_port_proxy = RubyPortProxy(ruby_system=system.ruby)
    system.system_port = system.sys_port_proxy.in_ports


def _instantiate_controllers(system, params):
    protocol = _protocols[params["protocol"]]
    protocol.instantiate(system.ruby, system.cpu, params)
    system.ruby.directory_controllers = [
        protocol.DirectoryController(system.ruby, i) for i in range(params["num_dirs"])
    ]
    for controller in system.ruby.controllers:
        controller.hmv_controller = system.hmv_controller
