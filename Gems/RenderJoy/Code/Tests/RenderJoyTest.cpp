/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzTest/AzTest.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <Source/RenderJoySystemComponent.h>

class RenderJoyTest
    : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Setup a system allocator
    }

    void TearDown() override
    {

    }
};

TEST_F(RenderJoyTest, RenderJoyIsHappy)
{
    ASSERT_TRUE(true);
}

AZ_UNIT_TEST_HOOK();
