/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Source/AutoGen/NetworkHierarchyChildComponent.AutoComponent.h>

namespace Multiplayer
{
    class NetworkHierarchyRootComponent;

    //! @class NetworkHierarchyChildComponent
    //! @brief Component that declares network dependency on the parent of this entity
    /*
     * The parent of this entity should have @NetworkHierarchyChildComponent (or @NetworkHierarchyRootComponent).
     * A network hierarchy is a collection of entities with one @NetworkHierarchyRootComponent at the top parent
     * and one or more @NetworkHierarchyChildComponent on its child entities.
     */
    class NetworkHierarchyChildComponent final
        : public NetworkHierarchyChildComponentBase
    {
        friend class NetworkHierarchyRootComponent;

    public:
        AZ_MULTIPLAYER_COMPONENT(Multiplayer::NetworkHierarchyChildComponent, s_networkHierarchyChildComponentConcreteUuid, Multiplayer::NetworkHierarchyChildComponentBase);

        static void Reflect(AZ::ReflectContext* context);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        NetworkHierarchyChildComponent();

        //! @{
        void OnInit() override;
        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        //! @}

        //! @note if the return value is nullptr, then this entity is not part of any hierarchy.
        //! One reason for that could be a missing @NetworkHierarchyChildComponent on an entity
        //! between this entity and the parent entity that has @NetworkHierarchyRootComponent on it.
        //!
        //! @returns top most @NetworkHierarchyRootComponent for the hierarchy
        const NetworkHierarchyRootComponent* GetHierarchyRootComponent() const;

        bool IsInHierarchy() const;

    protected:
        //! Used by @NetworkHierarchyRootComponent
        void SetTopLevelHierarchyRootComponent(NetworkHierarchyRootComponent* hierarchyRoot);

    private:
        const NetworkHierarchyRootComponent* m_hierarchyRootComponent = nullptr;

        AZ::Event<NetEntityId>::Handler m_hierarchyRootNetIdChanged;
        void OnHierarchyRootNetIdChanged(NetEntityId rootNetId);
    };
}
