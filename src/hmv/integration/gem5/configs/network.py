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

# Copyright (c) 2016 Georgia Institute of Technology
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

# gem5 imports
from m5.defines import buildEnv
from m5.objects import (
    GarnetNetwork,
    GarnetNetworkInterface,
    NetworkBridge,
    SimpleNetwork,
)

# internal imports
from topology import crossbar, pt2pt

_networks = {"simple": SimpleNetwork, "garnet": GarnetNetwork}
_topologies = {"crossbar": crossbar, "pt2pt": pt2pt}


def instantiate(ruby_system, params):
    assert params["network"] in _networks.keys(), "unsupported network type"
    assert params["topology"] in _topologies.keys(), "unsupported topology type"
    ruby_system.network = _networks[params["network"]](
        ruby_system=ruby_system,
        topology=params["topology"],
        number_of_virtual_networks=params["number_of_virtual_networks"],
        routers=[],
        ext_links=[],
        int_links=[],
        netifs=[],
    )
    _topologies[params["topology"]].connect_controllers(
        ruby_system.network,
        ruby_system.controllers + ruby_system.directory_controllers,
        params,
    )
    if params["network"] == "garnet":
        _initialize_garnet_network(ruby_system.network)
    else:
        _initialize_simple_network(ruby_system.network, params)


def _initialize_garnet_network(network):
    network.num_rows = 0  # args.mesh_rows
    network.vcs_per_vnet = 4  # args.garnet_vcs_per_vnet
    network.ni_flit_size = 128 / 8  # args.garnet_link_width_bits / 8
    network.routing_algorithm = 0  # args.garnet_routing_algorithm
    network.garnet_deadlock_threshold = 50000  # args.garnet_deadlock_threshold
    for intLink in network.int_links:
        intLink.src_net_bridge = NetworkBridge(
            link=intLink.network_link,
            vtype="OBJECT_LINK",
            width=intLink.src_node.width,
        )
        intLink.src_cred_bridge = NetworkBridge(
            link=intLink.credit_link,
            vtype="LINK_OBJECT",
            width=intLink.src_node.width,
        )
        intLink.dst_net_bridge = NetworkBridge(
            link=intLink.network_link,
            vtype="LINK_OBJECT",
            width=intLink.dst_node.width,
        )
        intLink.dst_cred_bridge = NetworkBridge(
            link=intLink.credit_link,
            vtype="OBJECT_LINK",
            width=intLink.dst_node.width,
        )
    for extLink in network.ext_links:
        extLink.ext_net_bridge = [
            NetworkBridge(
                link=extLink.network_links[0],
                vtype="OBJECT_LINK",
                width=extLink.width,
            ),
            NetworkBridge(
                link=extLink.network_links[1],
                vtype="LINK_OBJECT",
                width=extLink.width,
            ),
        ]
        extLink.ext_cred_bridge = [
            NetworkBridge(
                link=extLink.credit_links[0],
                vtype="LINK_OBJECT",
                width=extLink.width,
            ),
            NetworkBridge(
                link=extLink.credit_links[1],
                vtype="OBJECT_LINK",
                width=extLink.width,
            ),
        ]
        extLink.int_net_bridge = [
            NetworkBridge(
                link=extLink.network_links[0],
                vtype="LINK_OBJECT",
                width=extLink.int_node.width,
            ),
            NetworkBridge(
                link=extLink.network_links[1],
                vtype="OBJECT_LINK",
                width=extLink.int_node.width,
            ),
        ]
        extLink.int_cred_bridge = [
            NetworkBridge(
                link=extLink.credit_links[0],
                vtype="OBJECT_LINK",
                width=extLink.int_node.width,
            ),
            NetworkBridge(
                link=extLink.credit_links[1],
                vtype="LINK_OBJECT",
                width=extLink.int_node.width,
            ),
        ]
        network.netifs = [
            GarnetNetworkInterface(id=i) for i in range(len(network.ext_links))
        ]


def _initialize_simple_network(network, params):
    if params["simple_physical_channels"]:
        network.physical_vnets_channels = [1] * params["number_of_virtual_networks"]
    network.setup_buffers()
