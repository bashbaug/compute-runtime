/*
 * Copyright (C) 2017-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "common/helpers/bit_helpers.h"
#include "public/cl_ext_private.h"
#include "runtime/command_stream/preemption_mode.h"
#include "runtime/helpers/aligned_memory.h"
#include "runtime/helpers/engine_control.h"
#include "runtime/memory_manager/allocation_properties.h"
#include "runtime/memory_manager/gfx_partition.h"
#include "runtime/memory_manager/graphics_allocation.h"
#include "runtime/memory_manager/host_ptr_defines.h"
#include "runtime/memory_manager/local_memory_usage.h"
#include "runtime/os_interface/32bit_memory.h"

#include "engine_node.h"

#include <bitset>
#include <cstdint>
#include <mutex>
#include <vector>

namespace NEO {
class CommandStreamReceiver;
class DeferredDeleter;
class ExecutionEnvironment;
class Gmm;
class HostPtrManager;
class OsContext;

using CsrContainer = std::vector<std::vector<std::unique_ptr<CommandStreamReceiver>>>;
using EngineControlContainer = std::vector<EngineControl>;
using DeviceBitfield = std::bitset<32>;

inline DeviceBitfield getDeviceBitfieldForNDevices(uint32_t numDevices) {
    return DeviceBitfield((1u << numDevices) - 1u);
}

enum AllocationUsage {
    TEMPORARY_ALLOCATION,
    REUSABLE_ALLOCATION
};

struct AlignedMallocRestrictions {
    uintptr_t minAddress;
};

constexpr size_t paddingBufferSize = 2 * MemoryConstants::megaByte;

class MemoryManager {
  public:
    enum AllocationStatus {
        Success = 0,
        Error,
        InvalidHostPointer,
        RetryInNonDevicePool
    };

    MemoryManager(ExecutionEnvironment &executionEnvironment);

    virtual ~MemoryManager();
    MOCKABLE_VIRTUAL void *allocateSystemMemory(size_t size, size_t alignment);

    virtual void addAllocationToHostPtrManager(GraphicsAllocation *memory) = 0;
    virtual void removeAllocationFromHostPtrManager(GraphicsAllocation *memory) = 0;

    MOCKABLE_VIRTUAL GraphicsAllocation *allocateGraphicsMemoryWithProperties(const AllocationProperties &properties) {
        return allocateGraphicsMemoryInPreferredPool(properties, nullptr);
    }

    MOCKABLE_VIRTUAL GraphicsAllocation *allocateGraphicsMemoryWithProperties(const AllocationProperties &properties, const void *ptr) {
        return allocateGraphicsMemoryInPreferredPool(properties, ptr);
    }

    GraphicsAllocation *allocateGraphicsMemoryInPreferredPool(const AllocationProperties &properties, const void *hostPtr);

    virtual GraphicsAllocation *createGraphicsAllocationFromSharedHandle(osHandle handle, const AllocationProperties &properties, bool requireSpecificBitness) = 0;

    virtual GraphicsAllocation *createGraphicsAllocationFromNTHandle(void *handle) = 0;

    virtual bool mapAuxGpuVA(GraphicsAllocation *graphicsAllocation) { return false; }

    void *lockResource(GraphicsAllocation *graphicsAllocation);
    void unlockResource(GraphicsAllocation *graphicsAllocation);

    void cleanGraphicsMemoryCreatedFromHostPtr(GraphicsAllocation *);
    GraphicsAllocation *createGraphicsAllocationWithPadding(GraphicsAllocation *inputGraphicsAllocation, size_t sizeWithPadding);
    virtual GraphicsAllocation *createPaddedAllocation(GraphicsAllocation *inputGraphicsAllocation, size_t sizeWithPadding);

    virtual AllocationStatus populateOsHandles(OsHandleStorage &handleStorage) = 0;
    virtual void cleanOsHandles(OsHandleStorage &handleStorage) = 0;

    void freeSystemMemory(void *ptr);

    virtual void freeGraphicsMemoryImpl(GraphicsAllocation *gfxAllocation) = 0;
    void freeGraphicsMemory(GraphicsAllocation *gfxAllocation);
    virtual void handleFenceCompletion(GraphicsAllocation *allocation){};

    void checkGpuUsageAndDestroyGraphicsAllocations(GraphicsAllocation *gfxAllocation);

    virtual uint64_t getSystemSharedMemory() = 0;

    virtual uint64_t getMaxApplicationAddress() = 0;

    virtual uint64_t getInternalHeapBaseAddress() = 0;

    virtual uint64_t getExternalHeapBaseAddress() = 0;

    bool isLimitedRange() { return gfxPartition.isLimitedRange(); }

    bool peek64kbPagesEnabled() const { return enable64kbpages; }
    bool peekForce32BitAllocations() const { return force32bitAllocations; }
    virtual void setForce32BitAllocations(bool newValue);

    bool peekVirtualPaddingSupport() const { return virtualPaddingAvailable; }
    void setVirtualPaddingSupport(bool virtualPaddingSupport) { virtualPaddingAvailable = virtualPaddingSupport; }
    GraphicsAllocation *peekPaddingAllocation() { return paddingAllocation; }

    DeferredDeleter *getDeferredDeleter() const {
        return deferredDeleter.get();
    }

    void waitForDeletions();
    void waitForEnginesCompletion(GraphicsAllocation &graphicsAllocation);
    void cleanTemporaryAllocationListOnAllEngines(bool waitForCompletion);

    bool isAsyncDeleterEnabled() const;
    bool isLocalMemorySupported() const;
    virtual bool isMemoryBudgetExhausted() const;

    virtual AlignedMallocRestrictions *getAlignedMallocRestrictions() {
        return nullptr;
    }

    MOCKABLE_VIRTUAL void *alignedMallocWrapper(size_t bytes, size_t alignment) {
        return ::alignedMalloc(bytes, alignment);
    }

    MOCKABLE_VIRTUAL void alignedFreeWrapper(void *ptr) {
        ::alignedFree(ptr);
    }

    OsContext *createAndRegisterOsContext(CommandStreamReceiver *commandStreamReceiver, aub_stream::EngineType engineType,
                                          DeviceBitfield deviceBitfield, PreemptionMode preemptionMode, bool lowPriority);
    uint32_t getRegisteredEnginesCount() const { return static_cast<uint32_t>(registeredEngines.size()); }
    CommandStreamReceiver *getDefaultCommandStreamReceiver(uint32_t deviceId) const;
    EngineControlContainer &getRegisteredEngines();
    EngineControl *getRegisteredEngineForCsr(CommandStreamReceiver *commandStreamReceiver);
    HostPtrManager *getHostPtrManager() const { return hostPtrManager.get(); }
    void setDefaultEngineIndex(uint32_t index) { defaultEngineIndex = index; }
    virtual bool copyMemoryToAllocation(GraphicsAllocation *graphicsAllocation, const void *memoryToCopy, uint32_t sizeToCopy) const;
    static HeapIndex selectHeap(const GraphicsAllocation *allocation, bool hasPointer, bool isFullRangeSVM);
    static std::unique_ptr<MemoryManager> createMemoryManager(ExecutionEnvironment &executionEnvironment);
    virtual void *reserveCpuAddressRange(size_t size) = 0;
    virtual void releaseReservedCpuAddressRange(void *reserved, size_t size) = 0;

  protected:
    struct AllocationData {
        union {
            struct {
                uint32_t allocateMemory : 1;
                uint32_t allow64kbPages : 1;
                uint32_t allow32Bit : 1;
                uint32_t useSystemMemory : 1;
                uint32_t forcePin : 1;
                uint32_t uncacheable : 1;
                uint32_t flushL3 : 1;
                uint32_t preferRenderCompressed : 1;
                uint32_t multiOsContextCapable : 1;
                uint32_t requiresCpuAccess : 1;
                uint32_t reserved : 22;
            } flags;
            uint32_t allFlags = 0;
        };
        static_assert(sizeof(AllocationData::flags) == sizeof(AllocationData::allFlags), "");
        GraphicsAllocation::AllocationType type = GraphicsAllocation::AllocationType::UNKNOWN;
        const void *hostPtr = nullptr;
        size_t size = 0;
        size_t alignment = 0;
        StorageInfo storageInfo = {};
        ImageInfo *imgInfo = nullptr;
    };

    static bool getAllocationData(AllocationData &allocationData, const AllocationProperties &properties, const void *hostPtr, const StorageInfo &storageInfo);
    static bool useInternal32BitAllocator(GraphicsAllocation::AllocationType allocationType) {
        return allocationType == GraphicsAllocation::AllocationType::KERNEL_ISA ||
               allocationType == GraphicsAllocation::AllocationType::INTERNAL_HEAP;
    }
    StorageInfo createStorageInfoFromProperties(const AllocationProperties &properties);

    virtual GraphicsAllocation *createGraphicsAllocation(OsHandleStorage &handleStorage, const AllocationData &allocationData) = 0;
    virtual GraphicsAllocation *allocateGraphicsMemoryForNonSvmHostPtr(const AllocationData &allocationData) = 0;
    GraphicsAllocation *allocateGraphicsMemory(const AllocationData &allocationData);
    virtual GraphicsAllocation *allocateGraphicsMemoryWithHostPtr(const AllocationData &allocationData);
    virtual GraphicsAllocation *allocateGraphicsMemoryWithAlignment(const AllocationData &allocationData) = 0;
    virtual GraphicsAllocation *allocateGraphicsMemory64kb(const AllocationData &allocationData) = 0;
    virtual GraphicsAllocation *allocate32BitGraphicsMemoryImpl(const AllocationData &allocationData) = 0;
    virtual GraphicsAllocation *allocateGraphicsMemoryInDevicePool(const AllocationData &allocationData, AllocationStatus &status) = 0;
    GraphicsAllocation *allocateGraphicsMemoryForImageFromHostPtr(const AllocationData &allocationData);
    MOCKABLE_VIRTUAL GraphicsAllocation *allocateGraphicsMemoryForImage(const AllocationData &allocationData);
    virtual GraphicsAllocation *allocateGraphicsMemoryForImageImpl(const AllocationData &allocationData, std::unique_ptr<Gmm> gmm) = 0;
    virtual void *lockResourceImpl(GraphicsAllocation &graphicsAllocation) = 0;
    virtual void unlockResourceImpl(GraphicsAllocation &graphicsAllocation) = 0;
    virtual void freeAssociatedResourceImpl(GraphicsAllocation &graphicsAllocation) { return unlockResourceImpl(graphicsAllocation); };
    uint32_t getBanksCount();

    bool force32bitAllocations = false;
    bool virtualPaddingAvailable = false;
    GraphicsAllocation *paddingAllocation = nullptr;
    void applyCommonCleanup();
    std::unique_ptr<DeferredDeleter> deferredDeleter;
    bool asyncDeleterEnabled = false;
    bool enable64kbpages = false;
    bool localMemorySupported = false;
    bool supportsMultiStorageResources = true;
    ExecutionEnvironment &executionEnvironment;
    EngineControlContainer registeredEngines;
    std::unique_ptr<HostPtrManager> hostPtrManager;
    uint32_t latestContextId = std::numeric_limits<uint32_t>::max();
    uint32_t defaultEngineIndex = 0;
    std::unique_ptr<DeferredDeleter> multiContextResourceDestructor;
    std::unique_ptr<Allocator32bit> allocator32Bit;
    GfxPartition gfxPartition;
    std::unique_ptr<LocalMemoryUsageBankSelector> localMemoryUsageBankSelector;
};

std::unique_ptr<DeferredDeleter> createDeferredDeleter();
} // namespace NEO
