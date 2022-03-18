/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/Reflection/Attribute.h>
#include <AzFramework/DocumentPropertyEditor/Reflection/Visitor.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ::Reflection
{
    void VisitLegacyInMemoryInstance(IRead* visitor, void* instance, const AZ::TypeId& typeId);
    void VisitLegacyInMemoryInstance(IReadWrite* reader, void* instance, const AZ::TypeId& typeId);
    void VisitLegacyJsonSerializedInstance(IRead* visitor, Dom::Value instance, const AZ::TypeId& typeId);
}
