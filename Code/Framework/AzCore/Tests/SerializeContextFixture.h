/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    // This fixture provides a functional serialize context and allocators.
    class SerializeContextFixture : public AllocatorsFixture
    {
    protected:
        void SetUp() override;
        void TearDown() override;

    protected:
        AZ::SerializeContext* m_serializeContext = nullptr;
    };

    /*
     * Scoped RAII class automatically invokes the supplied reflection functions and reflects them to the supplied SerializeContext
     * On Destruction the serialize context is set to remove reflection and the reflection functions are invoked to to unreflect
     * them from the SerializeContext
     */
    class ScopedSerializeContextReflector
    {
    public:
        using ReflectCallable = AZStd::function<void(AZ::SerializeContext*)>;

        ScopedSerializeContextReflector(AZ::SerializeContext& serializeContext, std::initializer_list<ReflectCallable> reflectFunctions);
        ~ScopedSerializeContextReflector();

    private:
        AZ::SerializeContext& m_serializeContext;
        AZStd::vector<ReflectCallable> m_reflectFunctions;
    };
}
