/*
 * Copyright (C) 2018-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "shared/source/os_interface/os_memoryinfo.h"

#include <memory>

namespace NEO {

class OSInterface;

class OSMemoryInfoLinux : OSMemoryInfo {
  public:
    static std::unique_ptr<OSMemoryInfo> create(OSInterface *osInterface);
    OSMemoryInfoLinux(OSInterface *osInterface);

    bool getMemoryAllocInfo(uint64_t *free, uint64_t *total) const override;
};
} // namespace NEO
