/*
 * Copyright (C) 2017-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "runtime/built_ins/built_ins.h"
#include "runtime/command_queue/command_queue_hw.h"
#include "runtime/command_stream/command_stream_receiver.h"
#include "runtime/helpers/hardware_commands_helper.h"
#include "runtime/mem_obj/buffer.h"
#include "runtime/memory_manager/surface.h"

#include <new>

namespace NEO {

template <typename GfxFamily>
cl_int CommandQueueHw<GfxFamily>::enqueueCopyBufferRect(
    Buffer *srcBuffer,
    Buffer *dstBuffer,
    const size_t *srcOrigin,
    const size_t *dstOrigin,
    const size_t *region,
    size_t srcRowPitch,
    size_t srcSlicePitch,
    size_t dstRowPitch,
    size_t dstSlicePitch,
    cl_uint numEventsInWaitList,
    const cl_event *eventWaitList,
    cl_event *event) {

    MultiDispatchInfo dispatchInfo;

    auto &builder = getDevice().getExecutionEnvironment()->getBuiltIns()->getBuiltinDispatchInfoBuilder(EBuiltInOps::CopyBufferRect,
                                                                                                        this->getContext(), this->getDevice());
    BuiltInOwnershipWrapper builtInLock(builder, this->context);

    MemObjSurface srcBufferSurf(srcBuffer);
    MemObjSurface dstBufferSurf(dstBuffer);
    Surface *surfaces[] = {&srcBufferSurf, &dstBufferSurf};

    BuiltinDispatchInfoBuilder::BuiltinOpParams dc;
    dc.srcMemObj = srcBuffer;
    dc.dstMemObj = dstBuffer;
    dc.srcOffset = srcOrigin;
    dc.dstOffset = dstOrigin;
    dc.size = region;
    dc.srcRowPitch = srcRowPitch;
    dc.srcSlicePitch = srcSlicePitch;
    dc.dstRowPitch = dstRowPitch;
    dc.dstSlicePitch = dstSlicePitch;
    builder.buildDispatchInfos(dispatchInfo, dc);

    enqueueHandler<CL_COMMAND_COPY_BUFFER_RECT>(
        surfaces,
        false,
        dispatchInfo,
        numEventsInWaitList,
        eventWaitList,
        event);

    return CL_SUCCESS;
}
} // namespace NEO
