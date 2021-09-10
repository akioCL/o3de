/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>

namespace Multiplayer
{
    class NetworkHierarchyRootComponent;

    //! @class NetworkHierarchyChildComponent
    //! @brief Component that declares network dependency on the parent of this entity
    /*
     * When activated on the server, listens for parent changes and for the current parent, saves its parent dependency.
     * During serialization EntityReplicator should make note of the dependency and send to clients.
     * On a client, it should wait until that dependency has been activated.
     */
    class NetworkHierarchyChildComponent final
        : public AZ::Component
        , public AZ::TransformNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(NetworkHierarchyChildComponent, "{AB83BB6B-5BD5-4E84-9933-E30B79E36876}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        //! AZ::Component overrides.
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

        //! AZ::TransformNotificationBus::Handler overrides.
        //! @{
        void OnParentChanged(AZ::EntityId oldParent, AZ::EntityId newParent) override;
        //! @}

        void SetHierarchyRoot(NetworkHierarchyRootComponent* hierarchyRoot);
        NetworkHierarchyRootComponent* GetHierarchyRoot() const { return m_hierarchyRoot; }

    private:
        NetworkHierarchyRootComponent* m_hierarchyRoot = nullptr;
    };
}
