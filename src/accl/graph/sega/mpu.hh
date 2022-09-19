/*
 * Copyright (c) 2020 The Regents of the University of California.
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

#ifndef __ACCL_GRAPH_SEGA_MPU_HH__
#define __ACCL_GRAPH_SEGA_MPU_HH__

#include "accl/graph/base/data_structs.hh"
#include "accl/graph/sega/coalesce_engine.hh"
#include "accl/graph/sega/push_engine.hh"
#include "accl/graph/sega/wl_engine.hh"
#include "base/addr_range.hh"
#include "mem/packet.hh"
#include "mem/port.hh"
#include "sim/sim_object.hh"
#include "sim/system.hh"
#include "params/MPU.hh"

namespace gem5
{

class CenteralController;

class MPU : public SimObject
{
  private:
    class RespPort : public ResponsePort
    {
      private:
        MPU* owner;
        bool needSendRetryReq;

      public:
        RespPort(const std::string& name, MPU* owner):
          ResponsePort(name, owner), owner(owner), needSendRetryReq(false)
        {}
        virtual AddrRangeList getAddrRanges() const;

        void checkRetryReq();

      protected:
        virtual bool recvTimingReq(PacketPtr pkt);
        virtual Tick recvAtomic(PacketPtr pkt);
        virtual void recvFunctional(PacketPtr pkt);
        virtual void recvRespRetry();
    };

    class ReqPort : public RequestPort
    {
      private:
        MPU* owner;
        PacketPtr blockedPacket;

      public:
        ReqPort(const std::string& name, MPU* owner) :
          RequestPort(name, owner), owner(owner), blockedPacket(nullptr)
        {}
        void sendPacket(PacketPtr pkt);
        bool blocked() { return (blockedPacket != nullptr); }

      protected:
        virtual bool recvTimingResp(PacketPtr pkt);
        virtual void recvReqRetry();
    };

    System* system;
    CenteralController* centeralController;

    WLEngine* wlEngine;
    CoalesceEngine* coalesceEngine;
    PushEngine* pushEngine;

    RespPort inPort;
    ReqPort outPort;

    AddrRangeList localAddrRange;

  public:
    PARAMS(MPU);
    MPU(const Params& params);
    Port& getPort(const std::string& if_name,
                PortID idx = InvalidPortID) override;
    virtual void init() override;
    void registerCenteralController(CenteralController* centeral_controller);

    AddrRangeList getAddrRanges() { return coalesceEngine->getAddrRanges(); }
    void recvFunctional(PacketPtr pkt) { coalesceEngine->recvFunctional(pkt); }

    bool handleIncomingUpdate(PacketPtr pkt);
    void checkRetryReq() { inPort.checkRetryReq(); }
    void handleIncomingWL(Addr addr, WorkListItem wl);
    bool recvWLRead(Addr addr) { return coalesceEngine->recvWLRead(addr); }
    void recvWLWrite(Addr addr, WorkListItem wl);

    int workCount() { return coalesceEngine->workCount(); }
    void recvVertexPull() { return coalesceEngine->recvVertexPull(); }
    bool running() { return pushEngine->running(); }
    void start() { return pushEngine->start(); }
    void recvVertexPush(Addr addr, WorkListItem wl);

    bool blocked() { return outPort.blocked(); }
    void sendPacket(PacketPtr pkt);
    void recvReqRetry() { pushEngine->recvReqRetry(); }

    void recvDoneSignal();
    bool done();
};

} // namespace gem5

#endif // __ACCL_GRAPH_SEGA_MPU_HH__