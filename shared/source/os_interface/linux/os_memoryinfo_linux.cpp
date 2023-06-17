/*
 * Copyright (C) 2020-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/os_interface/linux/os_memoryinfo_linux.h"
#include "shared/source/os_interface/os_interface.h"

namespace NEO {

std::unique_ptr<OSMemoryInfo> OSMemoryInfoLinux::create(OSInterface *osInterface) {
    return std::unique_ptr<OSMemoryInfo>(new OSMemoryInfoLinux(osInterface));
}

OSMemoryInfoLinux::OSMemoryInfoLinux(OSInterface *osInterface) {
    this->osInterface = osInterface;
}

bool OSMemoryInfo::getMemoryAllocInfo(uint64_t *free, uint64_t *total) const
{
    *free = 999;
    *total = 888;
    return true;
}
} // namespace NEO
