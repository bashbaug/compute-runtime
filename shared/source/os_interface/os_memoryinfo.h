/*
 * Copyright (C) 2018-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include <memory>

namespace NEO {

class OSInterface;

class OSMemoryInfo {
  public:
    static std::unique_ptr<OSMemoryInfo> create(OSInterface *osInterface);
    OSMemoryInfo() = default;

    virtual ~OSMemoryInfo() = default;
    virtual bool getMemoryAllocInfo(uint64_t *free, uint64_t *total) const;
    OSInterface *getOSInterface() const {
        return osInterface;
    }

  protected:
    OSInterface *osInterface = nullptr;
};
} // namespace NEO
