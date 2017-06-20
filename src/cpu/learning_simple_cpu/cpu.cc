/*
 * Copyright (c) 2017 Jason Lowe-Power
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
 *
 * Authors: Jason Lowe-Power
 */

 #include "cpu/learning_simple_cpu/cpu.hh"

 #include "cpu/learning_simple_cpu/exec_context.hh"

#include "debug/LearningSimpleCPU.hh"

LearningSimpleCPU::LearningSimpleCPU(LearningSimpleCPUParams *params) :
    BaseCPU(params),
    port(name()+".port", this),
    instOutstanding(false), dataOutstanding(false), outstandingInst(nullptr),
    thread(this, 0, params->system, params->workload[0], params->itb,
           params->dtb, params->isa[0])
{
    fatal_if(FullSystem, "The LearningSimpleCPU doesn't support full system.");

    // Register this thread with the BaseCPU.
    threadContexts.push_back(thread.getTC());
}

void
LearningSimpleCPU::init()
{
    DPRINTF(LearningSimpleCPU, "LearningSimpleCPU init\n");

    BaseCPU::init();

    thread.getTC()->initMemProxies(thread.getTC());
}

void
LearningSimpleCPU::startup()
{
    DPRINTF(LearningSimpleCPU, "LearningSimpleCPU startup\n");

    BaseCPU::startup();

    thread.startup();
}

void
LearningSimpleCPU::wakeup(ThreadID tid)
{
    assert(tid == 0); // This CPU doesn't support more than one thread!

    // Activate the thread contexts
    if (thread.status() == ThreadContext::Suspended) {
        DPRINTF(LearningSimpleCPU,"[tid:%d] Suspended Processor awoke\n", tid);
        thread.activate();
    }
}

void
LearningSimpleCPU::activateContext(ThreadID tid)
{
    DPRINTF(LearningSimpleCPU, "ActivateContext thread: %d\n", tid);
    BaseCPU::activateContext(tid);

    thread.activate();
    schedule(new EventFunctionWrapper([this]{ fetchTranslate(); },
                                      name()+".initial_fetch",
                                      true),
             curTick());
}

void
LearningSimpleCPU::CPUPort::sendPacket(PacketPtr pkt)
{
    // Note: This flow control is very simple since the memobj is blocking.
    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");

    DPRINTF(LearningSimpleCPU, "Sending packet %s\n", pkt->print());

    // If we can't send the packet across the port, store it for later.
    if (!sendTimingReq(pkt)) {
        blockedPacket = pkt;
    }
}

bool
LearningSimpleCPU::CPUPort::recvTimingResp(PacketPtr pkt)
{
    // Just forward to the memobj.
    return owner->handleResponse(pkt);
}

void
LearningSimpleCPU::CPUPort::recvReqRetry()
{
    // We should have a blocked packet if this function is called.
    assert(blockedPacket != nullptr);

    DPRINTF(LearningSimpleCPU, "Got retry request.\n");

    // Grab the blocked packet.
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    // Try to resend it. It's possible that it fails again.
    sendPacket(pkt);
}

bool
LearningSimpleCPU::handleResponse(PacketPtr pkt)
{
    assert(dataOutstanding || instOutstanding);
    DPRINTF(LearningSimpleCPU, "Got response for addr %#x\n", pkt->getAddr());

    if (dataOutstanding) {
        dataOutstanding = false;
        memoryResponse(pkt);
    } else if (instOutstanding) {
        instOutstanding = false;
        executeInstruction(pkt);
    } else {
        panic("This is impossible");
    }

    return true;
}

void
LearningSimpleCPU::fetchTranslate()
{

    // TheISA::PCState pcState = thread.pcState();
    // pcState.microPC() confuses me.

    DPRINTF(LearningSimpleCPU, "Fetching addr %#x\n", thread.instAddr());

    RequestPtr req =
        new Request(0 /* asid */,
            thread.instAddr(), sizeof(TheISA::MachInst), Request::INST_FETCH,
            instMasterId(), thread.instAddr(), thread.contextId());

    TranslationState *translation = new TranslationState(*this);

    thread.itb->translateTiming(req, thread.getTC(), translation,
                                BaseTLB::Execute);
}

void
LearningSimpleCPU::fetchSend(RequestPtr req, const Fault &fault)
{
    panic_if(dataOutstanding || instOutstanding,
             "Should be no outstanding on fetch!");

    if (fault == NoFault) {
        DPRINTF(LearningSimpleCPU, "Sending fetch for addr %#x(pa: %#x)\n",
                req->getVaddr(), req->getPaddr());
        PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
        pkt->allocate();
        port.sendPacket(pkt);
        instOutstanding = true;
    } else {
        DPRINTF(LearningSimpleCPU, "Translation of addr %#x faulted\n",
                                   req->getVaddr());
        delete req;
        panic("Currently LearningSimpleCPU doesn't support fetch faults\n");
        // fetch fault: advance directly to next instruction (fault handler)
        // advanceInst(fault);
    }
}

void
LearningSimpleCPU::executeInstruction(PacketPtr pkt)
{
    DPRINTF(LearningSimpleCPU, "Decoding the instruction\n");
    // First, we need to decode
    thread.decoder.moreBytes(thread.pcState(), thread.instAddr(),
                      *pkt->getConstPtr<TheISA::MachInst>());
    TheISA::PCState next_pc = thread.pcState();
    StaticInstPtr inst = thread.decoder.decode(next_pc);

    panic_if(!inst, "Need to fetch more data, I guess. This is strange.");

    // Create an execution context for this instruction's execution.
    LearningSimpleContext exec_context(*this, thread, inst);

    if (inst->isMemRef()) {
        DPRINTF(LearningSimpleCPU, "Found a memory instruciton!\n");
        // Make the memory reference...
        Fault fault = inst->initiateAcc(&exec_context, nullptr);
    } else {
        DPRINTF(LearningSimpleCPU, "Found a non-memory instruciton!\n");
        // Execute the instruction...
        Fault fault = inst->execute(&exec_context, nullptr);

        // update the thread's PC to the nextPC
        TheISA::PCState pcState = thread.pcState();
        TheISA::advancePC(pcState, inst);
        thread.pcState(pcState);

        // Schedule an instruction fetch for the next cycle.
        schedule(new EventFunctionWrapper([this]{ fetchTranslate(); },
                                          name()+".initial_fetch",
                                          true),
                 nextCycle());
    }
}

void
LearningSimpleCPU::memoryTranslate(StaticInstPtr inst, uint8_t *data,
                                   Addr addr, unsigned int size,
                                   Request::Flags flags, uint64_t *res,
                                   bool read)
{
    DPRINTF(LearningSimpleCPU, "%s addr %#x (size: %d)\n",
            read ? "Read" : "Write", addr, size);

    // Check to see if access crosses cache line boundary.
    if (((addr + size - 1) / cacheLineSize()) > (addr / cacheLineSize())) {
        panic("CPU can't deal with accesses across a cache line boundary.");
    }

    RequestPtr req = new Request(0 /* asid */, addr, size, flags,
                        dataMasterId(), thread.instAddr(), thread.contextId());

    TranslationState *translation =
        new TranslationState(*this, inst, size, data, res);

    thread.dtb->translateTiming(req, thread.getTC(), translation,
                                read ? BaseTLB::Read : BaseTLB::Write);
}

void
LearningSimpleCPU::memorySend(StaticInstPtr inst, RequestPtr req,
                              const Fault &fault, uint8_t *data, uint64_t *res,
                              bool read)
{
    panic_if(dataOutstanding || instOutstanding,
             "Should be no outstanding on memory access!");

    if (fault != NoFault) {
        DPRINTF(LearningSimpleCPU, "Translation of addr %#x faulted\n",
                                   req->getVaddr());
        delete req;
        panic("Currently LearningSimpleCPU doesn't support fetch faults\n");
        return;
    }

    if (req->getFlags().isSet(Request::NO_ACCESS))
    {
        panic("Don't know how to deal with Request::NO_ACCESS");
    }

    PacketPtr pkt;
    if (read) {
        DPRINTF(LearningSimpleCPU, "Sending read for addr %#x(pa: %#x)\n",
                req->getVaddr(), req->getPaddr());
        pkt = Packet::createRead(req);
        pkt->allocate();
    } else {
        DPRINTF(LearningSimpleCPU, "Sending write for addr %#x(pa: %#x)\n",
                req->getVaddr(), req->getPaddr());
        panic_if(req->isLLSC() || req->isCondSwap(), "Can't do atomics");

        pkt = Packet::createWrite(req);
        // Assume we have data if it's a write.
        assert(data);
        pkt->dataDynamic<uint8_t>(data);
    }

    dataOutstanding = true;
    outstandingInst = inst;
    port.sendPacket(pkt);

}

void
LearningSimpleCPU::memoryResponse(PacketPtr pkt)
{
    assert(!pkt->isError());
    assert(outstandingInst);

    StaticInstPtr inst = outstandingInst;
    outstandingInst = nullptr;

    LearningSimpleContext exec_context(*this, thread, inst);

    Fault fault = inst->completeAcc(pkt, &exec_context, nullptr);

    panic_if(fault != NoFault, "Don't know how to handle this fault!");

    delete pkt->req;
    delete pkt;

    // Finally, we can move on to the next instruction.
    TheISA::PCState pcState = thread.pcState();
    TheISA::advancePC(pcState, inst);
    thread.pcState(pcState);

    // Schedule an instruction fetch for the next cycle.
    schedule(new EventFunctionWrapper([this]{ fetchTranslate(); },
                                      name()+".initial_fetch",
                                      true),
             nextCycle());
}

LearningSimpleCPU*
LearningSimpleCPUParams::create()
{
    return new LearningSimpleCPU(this);
}
