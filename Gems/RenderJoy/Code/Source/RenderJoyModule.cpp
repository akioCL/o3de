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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include "RenderJoyModule.h"
#include "RenderJoySystemComponent.h"
#include "Components/RenderJoyTextureEditorComponent.h"
#include "Components/RenderJoyKeyboardEditorComponent.h"
#include "Components/RenderJoyPassEditorComponent.h"
#include "Components/RenderJoySettingsEditorComponent.h"

namespace RenderJoy
{
    RenderJoyModule::RenderJoyModule()
    {
        m_descriptors.insert(m_descriptors.end(), {
            RenderJoySystemComponent::CreateDescriptor(),
            RenderJoyTextureEditorComponent::CreateDescriptor(),
            RenderJoyKeyboardEditorComponent::CreateDescriptor(),
            RenderJoyPassEditorComponent::CreateDescriptor(),
            RenderJoySettingsEditorComponent::CreateDescriptor(),
        });
    }

    AZ::ComponentTypeList RenderJoyModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<RenderJoySystemComponent>(),
        };
    }

}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(RenderJoy_deda12623060436ca264dec26835db06, RenderJoy::RenderJoyModule)
