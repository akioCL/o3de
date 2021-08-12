/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/UnitTest/TestTypes.h>

namespace AZ
{
    class ReflectionManager;
}

namespace UnitTest
{
    inline constexpr bool EnableLeakTracking = false;

    class RHITestFixture
        : public ScopedAllocatorSetupFixture
    {
        AZStd::unique_ptr<AZ::ReflectionManager> m_reflectionManager;

    public:
        RHITestFixture();

        AZ::SerializeContext* GetSerializeContext();

        ~RHITestFixture() override;

        void SetUp() override;

        void TearDown() override;
    };
}
