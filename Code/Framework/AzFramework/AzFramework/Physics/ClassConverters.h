/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/ShapeConfiguration.h>


namespace AZ
{
    namespace Serialization
    {
        class DataElementNode;
    }
}

namespace Physics
{
    namespace ClassConverters
    {
        bool RagdollNodeConfigConverter(AZ::SerializeContext& context, AZ::Serialization::DataElementNode& classElement);
        bool RagdollConfigConverter(AZ::SerializeContext& context, AZ::Serialization::DataElementNode& classElement);
        bool MaterialLibraryAssetConverter(AZ::SerializeContext& context, AZ::Serialization::DataElementNode& classElement);
        bool ColliderConfigurationConverter(AZ::SerializeContext& context, AZ::Serialization::DataElementNode& classElement);
        bool MaterialSelectionConverter(AZ::SerializeContext& context, AZ::Serialization::DataElementNode& classElement);
        
    } // namespace ClassConverters
} // namespace Physics
