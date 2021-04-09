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

from ..motherboards.abstract_motherboard import AbstractMotherboard

from typing import Optional

from m5.objects import DDR3_1600_8x8 as DIMM
from m5.objects import MemCtrl

from .abstract_memory import AbstractMemory
from ..motherboards.abstract_motherboard import AbstractMotherboard

class DDR3_1600_8x8(AbstractMemory):

    def __init__(self,
                size : Optional[str] = "512MiB",
                ) -> None:
        super(DDR3_1600_8x8, self).__init__(size = size)

        # The DDR3_1600_8x8 has a lot of variables with sensible defaults that
        # make sense for a DDR3_1600_8x8 device. Only the size has been
        # exposed.
        self._dram = DIMM(range = self.get_size_str())

    def incorporate_memory(self, motherboard: AbstractMotherboard) -> None:
        # Setup the memory controller and set the memory
        motherboard.get_system_simobject().mem_cntrls = [
            MemCtrl(dram = self._dram,
                    port = motherboard.get_membus().mem_side_ports)
        ]