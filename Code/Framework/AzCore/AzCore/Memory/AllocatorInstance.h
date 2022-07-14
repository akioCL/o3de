/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Class that controls an allocator's lifetime.
// Allocator creation scenarios:
// A1) initially in static memory through static initialization (before AZ::Environment is ready)
// A2) created for the first time after AZ::Environment is ready
// Once the AZ::Environment is ready, we transition allocator's data created in (A1) to allocators created
// by (A2).
// Allocator destruction will be controlled by AllocatorLifetime, since the environment will also
// wrap the MemoryAllocator with a AllocatorLifetime, then destruction is ref counted. Two scenarios:
// D1) if it was created during (A1), the allocator has to be destroyed by static deinitialization.
//     Static deinitialization order is inverse to static initialization order, since the first variable
//     that requires allocation will initialize the allocator, we are guaranteed to to get destroyed after
//     the last variable that requires to deallocate.
//     The environment will be destroyed first, but since the allocator is managed by ref counting, then it
//     will be held until the last AllocatorLifetime that holds a copy of the shared_ptr is around.
// D2) if it was created during (A2), since the environment also holds an AllocatorLifetime, then the
//     destruction will happen when the environment is destroyed. Since it was created after the environment
//     was created, it should be safe to destroy it when the environment is destroyed.
//     If in this scenario, there is a variable that holds an allocation to after the environment is destroyed,
//     the allocator would be destroyed at that time, which would lead to a leak. The variable's lifetime should
//     be managed properly to be freed before the allocator is destroyed. If that is not possible, e.g. because
//     the variable is static and the first allocation happens after the environment is ready, then the variable
//     should poke the allocator on construction to better define allocator's lifetime.
//
// NOTE: AllocatorLifetime is used through AllocatorInstance<AllocatorType>::StaticMembers::m_allocatorLifetime,
//       which is created in the AllocatorInstance<AllocatorType>::Get() method and called from
//       AllocatorInstance<AllocatorType>::Get(). Therefore the scope/lifetime is defined by where
//       AllocatorInstance<AllocatorType>::Get() is invoked from. Since this happens during a heap allocation
//       (before construction), this will be initialized before the variable that is being created. Because of
//       static deinitialization order being inverse to static initialization, this AllocatorLifetime will be
//       constructed before any variable using the allocator and will be destroyed after the variable is destroyed.
// NOTE: Allocator vtable transference between modules: suppose A.dll loads B.dll. Suppose Allocator1 is in AzCore
//       which is a static lib, linked by A.dll and B.dll. If B.dll uses Allocator1 first, then the vtable will be
//       pointing to the Allocator1's symbols from B.dll. If B.dll is unloaded, then A.dll wont be able to use the
//       allocator.
//       TODO: To solve this, we track the Module that created the allocator, when a module is about to be unloaded,
//       we make the module that is going to perform the unload to go through all allocators and check if any
//       allocator belongs to that module that is going to be unloaded, if it does, create a new allocator from the
//       module that will trigger the unload (A.dll in our example) and transfer the allocations.
//

#include <AzCore/Module/Environment.h>
#include <AzCore/Memory/AllocatorManager.h>

namespace AZ
{
    namespace Internal
    {
        template<class AllocatorType>
        class AllocatorInstanceStaticMembers;
    }

    template<class AllocatorType>
    class AllocatorInstance
    {
    public:
        static AllocatorType& Get(); // This is the current method AZ::AllocatorInstance uses to get the global allocator instances

        AllocatorInstance()
            : m_allocator(Environment::FindVariable<AllocatorType>(AzTypeInfo<AllocatorType>::Name()))
        {
            if (!m_allocator)
            {
                m_owner = true;
                m_allocator = Environment::CreateVariable<AllocatorType>(AZ_CRC_CE(AzTypeInfo<AllocatorType>::Name()));
                AZ::AllocatorManager::Instance().RegisterAllocator(&m_allocator.Get());
            }
        }

        AllocatorInstance(const AllocatorInstance&) = delete;
        AllocatorInstance(AllocatorInstance&&) = default;
        AllocatorInstance& operator=(const AllocatorInstance&) = delete;
        AllocatorInstance& operator=(AllocatorInstance&&) = default;

        ~AllocatorInstance()
        {
            if (m_owner)
            {
                AZ::AllocatorManager::Instance().UnRegisterAllocator(&m_allocator.Get());
            }
        }

        // Kept for backwards compatibility
        ////////////////////////////////////////////
        static bool IsReady()
        {
            return true;
        }

        static void Create()
        {
        }

        static void Destroy()
        {
            // To prevent leaks, we are going to garbage collect on destruction
            // Once we deprecate this function, we should 
            Get().GarbageCollect();
        }
    private:
        EnvironmentVariable<AllocatorType> m_allocator;
        bool m_owner = false;
    };
}

#include <AzCore/std/allocator_stateless.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Module/Environment.h>

namespace AZ
{
    template<class AllocatorType>
    AllocatorType& AllocatorInstance<AllocatorType>::Get() // This is the current method AZ::AllocatorInstance uses to get the global allocator instances
    {
        static AllocatorInstance<AllocatorType> allocator;
        return *allocator.m_allocator;
    }
}
