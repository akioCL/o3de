/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/ToString.h>
#include <AzCore/Serialization/EditContext.h>
#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/Components/NetworkHierarchyChildComponent.h>
#include <Multiplayer/Components/NetworkHierarchyRootComponent.h>

namespace Multiplayer
{
    void NetworkHierarchyChildComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<NetworkHierarchyChildComponent, AZ::Component>()
                ->Version(2);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<NetworkHierarchyChildComponent>(
                    "Network Hierarchy Child", "declares a network dependency on the root of this hierarchy")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Multiplayer")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ;
            }
        }
    }

    void NetworkHierarchyChildComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("NetworkTransformComponent"));
    }

    void NetworkHierarchyChildComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("NetworkHierarchyChildComponent"));
    }

    void NetworkHierarchyChildComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("NetworkHierarchyChildComponent"));
        incompatible.push_back(AZ_CRC_CE("NetworkHierarchyRootComponent"));
    }

    void NetworkHierarchyChildComponent::Activate()
    {
        AZ::EntityId parentId;
        AZ::TransformBus::EventResult(parentId, GetEntityId(), &AZ::TransformBus::Events::GetParentId);

        AZ::Vector3 worldPosition = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(worldPosition, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);

        AZ::Entity* actualParentEntity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(parentId);
        const bool actualParenHasHierarchy = actualParentEntity ? actualParentEntity->FindComponent<NetworkHierarchyRootComponent>() != nullptr : false;

        AZ_Printf("NetworkHierarchyChildComponent", "entity %s/[%s] activated @ %s, actual parent is %s/%s/%d",
            GetEntityId().ToString().c_str(),
            GetEntity()->GetName().c_str(),
            AZ::ToString(worldPosition).c_str(),
            parentId.ToString().c_str(),
            actualParentEntity ? actualParentEntity->GetName().c_str() : "not-found",
            actualParenHasHierarchy);
    }

    void NetworkHierarchyChildComponent::Deactivate()
    {
    }

    void NetworkHierarchyChildComponent::SetTopLevelHierarchyRoot(NetworkHierarchyRootComponent* hierarchyRoot)
    {
        m_hierarchyRoot = hierarchyRoot;
    }

    NetworkHierarchyRootComponent* NetworkHierarchyChildComponent::GetTopLevelHierarchyRoot() const
    {
        return m_hierarchyRoot;
    }
}
