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

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

namespace RenderJoy
{
    class RenderJoyModule
        : public  AZ::Module
    {
    public:
        AZ_RTTI(RenderJoyModule, "{DEDA1262-3060-436C-A264-DEC26835DB06}", AZ::Module);
        AZ_CLASS_ALLOCATOR(RenderJoyModule, AZ::SystemAllocator, 0);

        RenderJoyModule();

        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
