/*
 * Copyright (c) 2021 The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __DEV_LUPIO_LUPIO_TMR_HH__
#define __DEV_LUPIO_LUPIO_TMR_HH__

#include "arch/riscv/interrupts.hh"
#include "dev/io_device.hh"
#include "dev/platform.hh"
#include "dev/lupio/lupio_pic.hh"
#include "params/LupioTMR.hh"

namespace gem5
{

/**
 * LupioTMR:
 * A timer virtual device 
 */
class LupioPIC;

class LupioTMR : public BasicPioDevice
{
  private:
    const ByteOrder byteOrder = ByteOrder::little;
    System *system;
    int nThread;
    EventFunctionWrapper tmrEvent;

    Tick start = 0; 
    Tick next = 0; 
    Tick startTime = 0;

    // Register map
    enum
    {   
        LUPIO_TMR_TIME,
        LUPIO_TMR_LOAD,
        LUPIO_TMR_CTRL,
        LUPIO_TMR_STAT,

        // Max offset 
        LUPIO_TMR_MAX,
    };

    // Internal oscillator frequency
    uint32_t freq = 0;

    // Timer registers
    uint64_t reload = 0;

    // Control
    bool ie = false;
    bool pd = false;

    // Status
    bool expired = false;

    /**
     * Function to return data pertaining to the timer, such as the simulated
     * time in ticks
     */
    uint64_t lupioTMRRead(const uint8_t addr, int size);
    /**
     * Function to launch or stop the timer depending on the load value
     */
    void lupioTMRWrite(const uint8_t addr, uint64_t val64, int size);
    
    /**
     * Return the simulated time
     */
    uint64_t lupioTMRCurrentTime();
    /**
     * Schedule the next timer event
     */
    void lupioTMRSet();
    /**
     * Process the timer's event
     */
    void lupioTMRCallback();

    /**
     * Post or clear timer interrupts
     */
    void updateIRQ(int level);

  public:
    PARAMS(LupioTMR);
    LupioTMR(const Params &params);

    /**
     * Implement BasicPioDevice virtual functions
     */
    Tick read(PacketPtr pkt) override;
    Tick write(PacketPtr pkt) override;
};

} // namespace gem5

#endif // __DEV_LUPIO_LUPIO_TMR_HH_

