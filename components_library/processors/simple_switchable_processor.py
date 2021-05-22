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

from components_library.boards.abstract_board import AbstractBoard
from components_library.processors.simple_core import SimpleCore
from components_library.processors.cpu_types import CPUTypes
from .switchable_processor import SwitchableProcessor

from ..utils.override import *

from m5.util import warn

from m5.objects import KvmVM

from ..boards.coherence_protocol import is_ruby


class SimpleSwitchableProcessor(SwitchableProcessor):
    """
    A Simplified implementation of SwitchableProcessor where there is one
    processor at the start of the simuation, and another that can be switched
    to via the "switch" function later in the simulation. This is good for
    fast/detailed CPU setups.
    """

    def __init__(
        self,
        starting_core_type: CPUTypes,
        switch_core_type: CPUTypes,
        num_cores: int,
    ) -> None:

        if num_cores <= 0:
            raise AssertionError("Number of cores must be a positive integer!")

        self._start_key = "start"
        self._switch_key = "switch"
        self._current_is_start = True
        self._prepare_kvm = CPUTypes.KVM in (starting_core_type,
            switch_core_type)

        if starting_core_type in (CPUTypes.TIMING, CPUTypes.O3):
            self._mem_mode = "timing"
        elif starting_core_type == CPUTypes.KVM:
            self._mem_mode = "atomic_noncaching"
        elif starting_core_type == CPUTypes.ATOMIC:
            self._mem_mode = "atomic"
        else:
            raise NotImplementedError

        switchable_cores = {
            self._start_key: [
                SimpleCore(cpu_type=starting_core_type, core_id=i)
                for i in range(num_cores)
            ],
            self._switch_key: [
                SimpleCore(cpu_type=switch_core_type, core_id=i)
                for i in range(num_cores)
            ],
        }

        super(SimpleSwitchableProcessor, self).__init__(
            switchable_cores=switchable_cores,
            starting_cores=self._start_key,
        )

    @overrides(SwitchableProcessor)
    def incorporate_processor(self, board: AbstractBoard) -> None:
        super().incorporate_processor(board=board)

        if self._prepare_kvm:

            board.kvm_vm = KvmVM()

        board.mem_mode = self._mem_mode

        # print(bla)

    def switch(self):
        if self._current_is_start:
            self.switch_to_processor(self._switch_key)
        else:
            self.switch_to_processor(self._start_key)

        self._current_is_start = not self._current_is_start
