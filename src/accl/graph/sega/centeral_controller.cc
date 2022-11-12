/*
 * Copyright (c) 2021 The Regents of the University of California.
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

#include "accl/graph/sega/centeral_controller.hh"

#include <iostream>

#include "base/cprintf.hh"
#include "base/loader/memory_image.hh"
#include "base/loader/object_file.hh"
#include "debug/CenteralController.hh"
#include "mem/packet_access.hh"
#include "sim/sim_exit.hh"

namespace gem5
{

CenteralController::CenteralController(const Params& params):
    ClockedObject(params),
    system(params.system),
    mode(ProcessingMode::NOT_SET),
    state(BulkSynchronousState::DONT_CARE)
{
    for (auto mpu : params.mpu_vector) {
        mpuVector.push_back(mpu);
        mpu->registerCenteralController(this);
    }
}

void
CenteralController::createBFSWorkload(Addr init_addr, uint32_t init_value)
{
    workload = new BFSWorkload(init_addr, init_value);
}

// void
// CenteralController::createPRWorkload(float alpha, float threshold)
// {
//     workload = new PRWorkload(alpha, threshold);
// }

void
CenteralController::createPopCountDirectory(int atoms_per_block)
{
    fatal_if(mode == ProcessingMode::NOT_SET, "You should set the processing "
                        "mode by calling either setAsyncMode or setBSPMode.");
    if (mode == ProcessingMode::ASYNCHRONOUS) {
        for (auto mpu: mpuVector) {
            mpu->createAsyncPopCountDirectory(atoms_per_block);
        }
    }
    if (mode == ProcessingMode::BULK_SYNCHRONOUS) {
        for (auto mpu: mpuVector) {
            mpu->createBSPPopCountDirectory(atoms_per_block);
        }
    }
}

void
CenteralController::startup()
{
    for (auto mpu: mpuVector) {
        addrRangeListMap[mpu] = mpu->getAddrRanges();
        mpu->setProcessingMode(mode);
        mpu->recvWorkload(workload);
    }

    const auto& file = params().image_file;
    if (file == "")
        return;

    auto* object = loader::createObjectFile(file, true);
    fatal_if(!object, "%s: Could not load %s.", name(), file);

    loader::debugSymbolTable.insert(*object->symtab().globals());
    loader::MemoryImage image = object->buildImage();
    maxVertexAddr = image.maxAddr();

    PortProxy proxy(
    [this](PacketPtr pkt) {
        for (auto mpu: mpuVector) {
            AddrRangeList range_list = addrRangeListMap[mpu];
            if (contains(range_list, pkt->getAddr())) {
                mpu->recvFunctional(pkt);
            }
        }
    }, system->cacheLineSize());

    panic_if(!image.write(proxy), "%s: Unable to write image.");

    if (mode == ProcessingMode::BULK_SYNCHRONOUS) {
        state = BulkSynchronousState::CONSUMING;
    }
    for (auto mpu: mpuVector) {
        mpu->postMemInitSetup();
        if (!mpu->running() && (mpu->workCount() > 0)) {
            mpu->start();
        }
    }
}

PacketPtr
CenteralController::createReadPacket(Addr addr, unsigned int size)
{
    RequestPtr req = std::make_shared<Request>(addr, size, 0, 0);
    // Dummy PC to have PC-based prefetchers latch on; get entropy into higher
    // bits
    req->setPC(((Addr) 0) << 2);

    // Embed it in a packet
    PacketPtr pkt = new Packet(req, MemCmd::ReadReq);
    pkt->allocate();

    return pkt;
}

void
CenteralController::recvDoneSignal()
{
    bool done = true;
    for (auto mpu : mpuVector) {
        done &= mpu->done();
    }

    if (done && mode == ProcessingMode::ASYNCHRONOUS) {
        exitSimLoopNow("no update left to process.");
    }

    if (done && mode == ProcessingMode::BULK_SYNCHRONOUS) {
        assert(state != BulkSynchronousState::DONT_CARE);
        if (state == BulkSynchronousState::APPLYING) {
            // TODO:
            // 1- Toggle directories
            // 2- Check if termination condition is met
            // 3- If yes, schedule exit event,
            // 4- If not switch state to consuming.
            exitSimLoopNow("applying done.");
        } else if (state == BulkSynchronousState::CONSUMING) {
            // TODO:
            // Schedule Bulk apply
            exitSimLoopNow("consuming done.");
        }
    }
}

void
CenteralController::printAnswerToHostSimout()
{
    int num_items = system->cacheLineSize() / sizeof(WorkListItem);
    WorkListItem items[num_items];
    for (Addr addr = 0; addr < maxVertexAddr; addr += system->cacheLineSize())
    {
        PacketPtr pkt = createReadPacket(addr, system->cacheLineSize());
        for (auto mpu: mpuVector) {
            AddrRangeList range_list = addrRangeListMap[mpu];
            if (contains(range_list, addr)) {
                mpu->recvFunctional(pkt);
            }
        }
        pkt->writeDataToBlock((uint8_t*) items, system->cacheLineSize());
        for (int i = 0; i < num_items; i++) {
            workload->apply(items[i]);
            std::string print = csprintf("WorkListItem[%lu][%d]: %s.", addr, i,
                                        workload->printWorkListItem(items[i]));

            std::cout << print << std::endl;
        }
    }
}

}
