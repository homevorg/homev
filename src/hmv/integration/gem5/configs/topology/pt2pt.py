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

# Copyright (c) 2011 Advanced Micro Devices, Inc.
#               2011 Massachusetts Institute of Technology
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
from m5.objects import (
    GarnetExtLink,
    GarnetIntLink,
    GarnetRouter,
    SimpleExtLink,
    SimpleIntLink,
    Switch,
)


def connect_controllers(network, controllers, params):
    network.routers = _instantiate_routers(len(controllers), params)
    ExtLinkClass, IntLinkClass = _get_classes(params)
    network.ext_links = [
        ExtLinkClass(
            link_id=i,
            ext_node=controller,
            int_node=network.routers[i],
            latency=params["link_latency"],
        )
        for i, controller in enumerate(controllers)
    ]
    link_pairs = [
        (source, destination)
        for source in range(len(controllers))
        for destination in range(len(controllers))
        if source != destination
    ]
    network.int_links = [
        IntLinkClass(
            link_id=len(controllers) + i,
            src_node=network.routers[pair[0]],
            dst_node=network.routers[pair[1]],
            latency=params["link_latency"],
        )
        for i, pair in enumerate(link_pairs)
    ]
    for controller in controllers:
        controller.connect_buffers(network)


def _instantiate_routers(num_routers, params):
    if params["network"] == "garnet":
        return [
            GarnetRouter(router_id=i, latency=params["garnet_router_latency"])
            for i in range(num_routers)
        ]
    return [Switch(router_id=i) for i in range(num_routers)]


def _get_classes(params):
    if params["network"] == "garnet":
        return GarnetExtLink, GarnetIntLink
    return SimpleExtLink, SimpleIntLink
