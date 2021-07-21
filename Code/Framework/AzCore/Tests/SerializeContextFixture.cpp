/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SerializeContextFixture.h"
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace UnitTest
{
    void SerializeContextFixture::SetUp()
    {
        AllocatorsFixture::SetUp();

        AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

        m_serializeContext = aznew AZ::SerializeContext(true, true);
    }

    void SerializeContextFixture::TearDown()
    {
        delete m_serializeContext;
        m_serializeContext = nullptr;

        AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();

        AllocatorsFixture::TearDown();
    }

    ScopedSerializeContextReflector::ScopedSerializeContextReflector(AZ::SerializeContext& serializeContext, std::initializer_list<ReflectCallable> reflectFunctions)
        : m_serializeContext(serializeContext)
        , m_reflectFunctions(reflectFunctions)
    {
        bool isCurrentlyRemovingReflection = m_serializeContext.IsRemovingReflection();
        if (isCurrentlyRemovingReflection)
        {
            m_serializeContext.DisableRemoveReflection();
        }
        for (ReflectCallable& reflectFunction : m_reflectFunctions)
        {
            if (reflectFunction)
            {
                reflectFunction(&m_serializeContext);
            }
        }
        if (isCurrentlyRemovingReflection)
        {
            m_serializeContext.EnableRemoveReflection();
        }
    }

    ScopedSerializeContextReflector::~ScopedSerializeContextReflector()
    {
        // Unreflects reflected functions in reverse order
        bool isCurrentlyRemovingReflection = m_serializeContext.IsRemovingReflection();
        if (!isCurrentlyRemovingReflection)
        {
            m_serializeContext.EnableRemoveReflection();
        }
        for (auto reflectFunctionIter = m_reflectFunctions.rbegin(); reflectFunctionIter != m_reflectFunctions.rend(); ++reflectFunctionIter)
        {
            ReflectCallable& reflectFunction = *reflectFunctionIter;
            if (reflectFunction)
            {
                reflectFunction(&m_serializeContext);
            }
        }
        if (!isCurrentlyRemovingReflection)
        {
            m_serializeContext.DisableRemoveReflection();
        }
    }
}
