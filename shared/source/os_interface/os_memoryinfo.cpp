/*
 * Copyright (C) 2020-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/helpers/driver_model_type.h"
#include "shared/source/os_interface/linux/os_memoryinfo_linux.h"
#include "shared/source/os_interface/os_interface.h"
#include "shared/source/os_interface/os_memoryinfo.h"

namespace NEO {

std::unique_ptr<OSMemoryInfo> OSMemoryInfo::create(OSInterface *osInterface) {
    if (osInterface) {
        if (osInterface->getDriverModel()->getDriverModelType() == DriverModelType::DRM) {
            return OSMemoryInfoLinux::create(osInterface);
        }
    }
    return std::make_unique<OSMemoryInfo>();
}

bool OSMemoryInfo::getMemoryAllocInfo(uint64_t *free, uint64_t *total) const
{
    return false;
}
} // namespace NEO
