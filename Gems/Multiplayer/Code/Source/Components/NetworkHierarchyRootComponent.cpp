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

AZ_CVAR(uint32_t, bg_hierarchyEntityMaxLimit, 16, nullptr, AZ::ConsoleFunctorFlags::Null,
    "Maximum allowed size of network entity hierarchies");

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
        const AZ::EntityId entityBusId = *AZ::TransformNotificationBus::GetCurrentBusId();
        if (GetEntityId() != entityBusId)
        {
            return; // ignore parent changes of child entities
        }

        if (AZ::Entity* parentEntity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(newParent))
        {
            if (parentEntity->FindComponent<NetworkHierarchyRootComponent>())
            {
                m_higherRoot = parentEntity;
                m_children.clear();
                AZ::TransformNotificationBus::MultiHandler::BusDisconnect();

                // Should still listen for its events, such as when this root detaches
                AZ::TransformNotificationBus::MultiHandler::BusConnect(GetEntityId());
            }
        }
        else
        {
            // detached from parent
            m_children.clear();
            AZ::TransformNotificationBus::MultiHandler::BusDisconnect();

            AZ::TransformNotificationBus::MultiHandler::BusConnect(GetEntityId());

            uint32_t currentEntityCount = 0;
            AttachHierarchicalChild(GetEntityId(), currentEntityCount);
        }
    }

    void NetworkHierarchyRootComponent::OnChildAdded(AZ::EntityId child)
    {
        uint32_t currentEntityCount = 0;

        if (AZ::Entity* childEntity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(child))
        {
            auto* hierarchyChildComponent = childEntity->FindComponent<NetworkHierarchyChildComponent>();
            auto* hierarchyRootComponent = childEntity->FindComponent<NetworkHierarchyRootComponent>();

            if (hierarchyChildComponent || hierarchyRootComponent)
            {
                AZ::TransformNotificationBus::MultiHandler::BusConnect(child);
                m_children.push_back(childEntity);
                ++currentEntityCount;

                if (hierarchyChildComponent)
                {
                    hierarchyChildComponent->SetHierarchyRoot(this);
                }
                else if (hierarchyRootComponent)
                {
                    hierarchyRootComponent->SetHierarchyRoot(GetEntity());
                }

                AttachHierarchicalChild(child, currentEntityCount);
            }
        }
    }

    void NetworkHierarchyRootComponent::AttachHierarchicalChild(AZ::EntityId underEntity, uint32_t& currentEntityCount)
    {
        AZStd::vector<AZ::EntityId> allChildren;
        AZ::TransformBus::EventResult(allChildren, underEntity, &AZ::TransformBus::Events::GetChildren);

        for (AZ::EntityId newChildId : allChildren)
        {
            if (currentEntityCount >= bg_hierarchyEntityMaxLimit)
            {
                break;
            }

            if (AZ::Entity* childEntity = AZ::Interface<AZ::ComponentApplicationRequests>::Get()->FindEntity(newChildId))
            {
                auto* hierarchyChildComponent = childEntity->FindComponent<NetworkHierarchyChildComponent>();
                auto* hierarchyRootComponent = childEntity->FindComponent<NetworkHierarchyRootComponent>();

                if (hierarchyChildComponent || hierarchyRootComponent)
                {
                    AZ::TransformNotificationBus::MultiHandler::BusConnect(newChildId);
                    m_children.push_back(childEntity);
                    ++currentEntityCount;

                    if (hierarchyChildComponent)
                    {
                        hierarchyChildComponent->SetHierarchyRoot(this);
                    }
                    else if (hierarchyRootComponent)
                    {
                        hierarchyRootComponent->SetHierarchyRoot(GetEntity());
                    }

                    AttachHierarchicalChild(newChildId, currentEntityCount);
                }
            }
        }
    }

    void NetworkHierarchyRootComponent::OnChildRemoved(AZ::EntityId childRemovedId)
    {
        AZStd::vector<AZ::EntityId> allChildren;
        AZ::TransformBus::EventResult(allChildren, childRemovedId, &AZ::TransformBus::Events::GetEntityAndAllDescendants);

        for (AZ::EntityId childId : allChildren)
        {
            AZ::TransformNotificationBus::MultiHandler::BusDisconnect(childId);

            const AZ::Entity* childEntity = nullptr;

            AZStd::erase_if(m_children, [childId, &childEntity](const AZ::Entity* entity)
                {
                    if (entity->GetId() == childId)
                    {
                        childEntity = entity;
                        return true;
                    }

                    return false;
                });

            if (childEntity)
            {
                if (NetworkHierarchyChildComponent* childComponent = childEntity->FindComponent<NetworkHierarchyChildComponent>())
                {
                    childComponent->SetHierarchyRoot(nullptr);
                }
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
