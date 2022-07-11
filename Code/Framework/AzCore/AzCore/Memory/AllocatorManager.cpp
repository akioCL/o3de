/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/Memory.h>

#include <AzCore/Memory/OSAllocator.h>

#include <AzCore/std/containers/array.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AZ
{
    static EnvironmentVariable<AllocatorManager> s_allocManager = nullptr;
    static AllocatorManager* s_allocManagerDebug = nullptr; // For easier viewing in crash dumps

    AllocatorManager::~AllocatorManager() = default;

    // The only allocator manager instance.
    AllocatorManager& AllocatorManager::Instance()
    {
        // Potentially need a similar method to AllocatorInstance to keep this data around. For the time being, we register the
        // allocator once is moved to the environment, however, that means that allocators may still be around once the AllocatorManager
        // is destroyed
        if (!s_allocManager)
        {
            s_allocManager = Environment::CreateVariable<AllocatorManager>(AZ_CRC_CE("AZ::AllocatorManager::s_allocManager"));
            s_allocManagerDebug = &(*s_allocManager);
        }
        return *s_allocManager;
    }

    void AllocatorManager::GarbageCollect()
    {
        size_t numAllocators;
        AZStd::array<IAllocator*, s_maxNumAllocators> allocators;
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
            numAllocators = m_numAllocators;
            allocators = m_allocators;
        }
        for (size_t i = 0; i < numAllocators; ++i)
        {
            IAllocator* allocator = allocators[i];
            allocator->GarbageCollect();
        }
    }

    void AllocatorManager::RegisterAllocator(IAllocator* alloc)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
        AZ_Assert(m_numAllocators < s_maxNumAllocators, "Too many allocators %d! Max is %d", m_numAllocators, s_maxNumAllocators);

#if defined(AZ_ENABLE_TRACING)
        for (size_t i = 0; i < m_numAllocators; i++)
        {
            AZ_Assert(m_allocators[i] != alloc, "Allocator %p registered twice", alloc);
        }
#endif
        m_allocators[m_numAllocators++] = alloc;
    }

    void AllocatorManager::UnRegisterAllocator(IAllocator* alloc)
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
        for (int i = 0; i < m_numAllocators; ++i)
        {
            if (m_allocators[i] == alloc)
            {
                --m_numAllocators;
                m_allocators[i] = m_allocators[m_numAllocators];
                return;
            }
        }
        AZ_Assert(false, "Allocator %p not registered", alloc)
    }

    void AllocatorManager::GetAllocatorStats(size_t& allocatedBytes, size_t& capacityBytes, AZStd::vector<AllocatorStats>* outStats)
    {
        allocatedBytes = 0;
        capacityBytes = 0;

        AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);

        // Build a mapping of original allocator sources to their allocators
        for (const IAllocator* allocator : m_allocators)
        {
            allocatedBytes += allocator->NumAllocatedBytes();
            capacityBytes += allocator->Capacity();

            if (outStats)
            {
                outStats->emplace(outStats->end(),
                    allocator->GetName(),
                    allocator->NumAllocatedBytes(),
                    allocator->Capacity());
            }
        }
    }
} // namespace AZ
