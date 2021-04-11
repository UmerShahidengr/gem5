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

from abc import ABC

from m5.defines import buildEnv
from m5.objects import System, BadAddr, BaseXBar

from .isas import ISA
from .coherence_protocol import CoherenceProtocol

class AbstractMotherboard(ABC):
    def __init__(self,
                 processor: "AbstractProcessor",
                 memory: "AbstractMemory",
                 cache_hierarchy: "AbstractCacheHierarchy",
                 membus: "BaseXBar",
                ) -> None:

        self._processor = processor
        self._memory = memory
        self._cache_hierarchy = cache_hierarchy
        self._system = System()

        # Create the main memory bus
        # This connects to main memory
        # TODO: This probably shouldn't exist in the abstract
        # self._system.membus = membus
        # self._system.membus.badaddr_responder = BadAddr()
        # self._system.membus.default = \
        #     self._system.membus.badaddr_responder.pio

        # Set up the system port for functional access from the simulator.
        # self._system.system_port = self._system.membus.cpu_side_ports

    def get_processor(self) -> "AbstractProcessor":
        return self._processor

    def get_memory(self) -> "AbstractMemory":
        return self._memory

    def get_cache_hierarchy(self) -> "AbstractCacheHierarchy":
        return self._cache_hierarchy

    def get_system_simobject(self) -> System:
        return self._system

    def get_membus(self) -> BaseXBar:
        return self._system.membus

    def get_runtime_isa(self) -> ISA:
        isa_map = {
            "sparc" : ISA.SPARC,
            "mips" : ISA.MIPS,
            "null" : ISA.NULL,
            "arm" : ISA.ARM,
            "x86" : ISA.X86,
            "power" : ISA.POWER,
            "riscv" : ISA.RISCV,
        }

        isa_str = str(buildEnv['TARGET_ISA']).lower()
        if isa_str not in isa_map.keys():
            raise NotImplementedError("ISA '" + buildEnv['TARGET_ISA'] +
                                      "' not recognized.")

        return isa_map[isa_str]



    def get_runtime_coherence_protocol(self) -> CoherenceProtocol:
        protocol_map = {
            "mi_example" : CoherenceProtocol.MI_EXAMPLE,
            "moesi_hammer" : CoherenceProtocol.ARM_MOESI_HAMMER,
            "garnet_standalone" : CoherenceProtocol.GARNET_STANDALONE,
            "moesi_cmp_token" : CoherenceProtocol.MOESI_CMP_TOKEN,
            "mesi_two_level" : CoherenceProtocol.MESI_TWO_LEVEL,
            "moesi_amd_base" : CoherenceProtocol.MOESI_AMD_BASE,
            "mesi_three_level_htm" : CoherenceProtocol.MESI_THREE_LEVEL_HTM,
            "mesi_three_level" : CoherenceProtocol.MESI_THREE_LEVEL,
            "gpu_viper" : CoherenceProtocol.GPU_VIPER,
            "chi" : CoherenceProtocol.CHI,
        }

        protocol_str = str(buildEnv['PROTOCOL']).lower()
        if protocol_str not in protocol_map.keys():
            raise NotImplementedError("Protocol '" + buildEnv['PROTOCOL'] +
                                      "' not recognized.")

        return protocol_map[protocol_str]
