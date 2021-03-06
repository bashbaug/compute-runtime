/*
 * Copyright (C) 2017-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "runtime/command_stream/tbx_command_stream_receiver.h"

#include "runtime/execution_environment/execution_environment.h"
#include "runtime/helpers/hw_info.h"
#include "runtime/helpers/options.h"

#include <string>

namespace NEO {

TbxCommandStreamReceiverCreateFunc tbxCommandStreamReceiverFactory[IGFX_MAX_CORE] = {};

CommandStreamReceiver *TbxCommandStreamReceiver::create(const std::string &baseName, bool withAubDump, ExecutionEnvironment &executionEnvironment) {
    auto hwInfo = executionEnvironment.getHardwareInfo();

    if (hwInfo->platform.eRenderCoreFamily >= IGFX_MAX_CORE) {
        DEBUG_BREAK_IF(!false);
        return nullptr;
    }

    auto pCreate = tbxCommandStreamReceiverFactory[hwInfo->platform.eRenderCoreFamily];

    return pCreate ? pCreate(baseName, withAubDump, executionEnvironment) : nullptr;
}
} // namespace NEO
