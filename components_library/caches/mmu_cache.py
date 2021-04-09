# Copyright (c) 2021 The Regents of the University of California
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

from m5.objects import Cache, L2XBar, BaseXBar, BaseCPU

from typing import Optional, Union


class MMUCache(Cache):

    def __init__(self,
                size: Optional[str] = '8kB',
                assoc: Optional[int] = 4,
                tag_latency: Optional[int] = 1,
                data_latency: Optional[int] = 1,
                response_latency: Optional[int] = 1,
                mshrs: Optional[int] = 20,
                tgts_per_mshr: Optional[int] = 12,
                writeback_clean: Optional[bool] = True):
        super(MMUCache, self).__init__(
                                       size = size,
                                       assoc= assoc,
                                       tag_latency=tag_latency,
                                       data_latency=data_latency,
                                       response_latency=response_latency,
                                       mshrs=mshrs,
                                       tgts_per_mshr=tgts_per_mshr,
                                       writeback_clean=writeback_clean
                                       )


    def connect_bus_side(self, bus_side: BaseXBar) -> None:
        """Connect this cache to a memory-side bus"""
        self.mem_side = bus_side.cpu_side_ports

    def connect_cpu_side(self, cpu_side: Union[BaseXBar, BaseCPU]) -> None:
        """Connect the CPU itb and dtb to the cache
           Note: This creates a new crossbar
        """
        self.mmubus = L2XBar()
        self.cpu_side = self.mmubus.mem_side_ports
        cpu_side.mmu.connectWalkerPorts(self.mmubus.cpu_side_ports,
                                        self.mmubus.cpu_side_ports)