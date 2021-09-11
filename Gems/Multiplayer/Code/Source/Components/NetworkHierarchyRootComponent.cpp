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

#pragma optimize("", off)

namespace Multiplayer
{
    void NetworkHierarchyRootComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<NetworkHierarchyRootComponent, AZ::Component>()
                ->Version(1);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<NetworkHierarchyRootComponent>(
                    "Network Hierarchy Root", "marks the entity as the root of an entity hierarchy")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Multiplayer")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ;
            }
        }
    }

    void NetworkHierarchyRootComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("NetworkTransformComponent"));
    }

    void NetworkHierarchyRootComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("NetworkHierarchyRootComponent"));
    }

    void NetworkHierarchyRootComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("NetworkHierarchyChildComponent"));
        incompatible.push_back(AZ_CRC_CE("NetworkHierarchyRootComponent"));
    }

    void NetworkHierarchyRootComponent::Activate()
    {
        AZ::TransformNotificationBus::MultiHandler::BusConnect(GetEntityId());

        AZ::EntityId parentId;
        AZ::TransformBus::EventResult(parentId, GetEntityId(), &AZ::TransformBus::Events::GetParentId);

        AZ::Vector3 worldPosition = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(worldPosition, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);

        AZ::Entity* actualParentEntity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(parentId);
        const bool actualParenHasHierarchy = actualParentEntity ? actualParentEntity->FindComponent<NetworkHierarchyRootComponent>() != nullptr : false;

        AZ_Printf("NetworkHierarchyRootComponent", "entity %s/[%s] activated @ %s, actual parent is %s/%s/%d",
            GetEntityId().ToString().c_str(), GetEntity()->GetName().c_str(), AZ::ToString(worldPosition).c_str(),
            parentId.ToString().c_str(), actualParentEntity ? actualParentEntity->GetName().c_str() : "not-found", actualParenHasHierarchy);
    }

    void NetworkHierarchyRootComponent::Deactivate()
    {
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect();
    }

    void NetworkHierarchyRootComponent::OnParentChanged([[maybe_unused]] AZ::EntityId oldParent, AZ::EntityId newParent)
    {
        if (AZ::Entity* parentEntity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(newParent))
        {
            const ConstNetworkEntityHandle parentHandle(parentEntity, GetNetworkEntityTracker());
            //parentHandle.GetNetEntityId()

            AZ_Printf("NetworkHierarchyRootComponent", "new parent %s", parentEntity->GetName().c_str());
        }
    }

    void NetworkHierarchyRootComponent::OnChildAdded(AZ::EntityId child)
    {
        AZStd::vector<AZ::EntityId> allChildren;
        AZ::TransformBus::EventResult(allChildren, child, &AZ::TransformBus::Events::GetEntityAndAllDescendants);

        for (AZ::EntityId newChildId : allChildren)
        {
            if (AZ::Entity* childEntity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(newChildId))
            {
                if (auto* hierarchyChildComponent = childEntity->FindComponent<NetworkHierarchyChildComponent>())
                {
                    AZ::TransformNotificationBus::MultiHandler::BusConnect(child);

                    m_children.push_back(childEntity);
                    hierarchyChildComponent->SetHierarchyRoot(this);
                }
                else if (auto* hierarchyRootComponent = childEntity->FindComponent<NetworkHierarchyRootComponent>())
                {
                    // Do not listen for children of other roots, just stop at the root

                    m_children.push_back(childEntity);
                    hierarchyRootComponent->SetHierarchyRoot(this);
                }
            }
        }
    }

    void NetworkHierarchyRootComponent::OnChildRemoved(AZ::EntityId child)
    {
        AZStd::vector<AZ::EntityId> allChildren;
        AZ::TransformBus::EventResult(allChildren, child, &AZ::TransformBus::Events::GetEntityAndAllDescendants);

        for (AZ::EntityId childId : allChildren)
        {
            if (const auto childIterator = AZStd::find_if(m_children.begin(), m_children.end(), [childId](const AZ::Entity* entity)
                {
                    return entity->GetId() == childId;
                }))
            {
                const AZ::Entity* childEntity = *childIterator;

                if (NetworkHierarchyChildComponent* childComponent = childEntity->FindComponent<NetworkHierarchyChildComponent>())
                {
                    childComponent->SetHierarchyRoot(nullptr);
                }

                m_children.erase(childIterator);
                AZ::TransformNotificationBus::MultiHandler::BusDisconnect(childId);
            }
        }
    }

    bool NetworkHierarchyRootComponent::IsAttachedToAnotherHierarchy() const
    {
        if (GetEntity()->GetTransform())
        {
            const AZ::EntityId parentId = GetEntity()->GetTransform()->GetParentId();
            if (parentId.IsValid())
            {
                const AZ::Entity* parentEntity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(parentId);
                if (parentEntity)
                {
                    auto* hierarchyRootComponent = parentEntity->FindComponent<NetworkHierarchyRootComponent>();
                    auto* hierarchyChildComponent = parentEntity->FindComponent<NetworkHierarchyChildComponent>();

                    if (hierarchyRootComponent || hierarchyChildComponent)
                    {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    const AZStd::vector<AZ::Entity*>& NetworkHierarchyRootComponent::GetHierarchyChildren() const
    {
        return m_children;
    }
}
