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
    //! @class NetworkHierarchyRootComponent
    //! @brief Component that declares network dependency on the parent of this entity
    /*
     * When activated on the server, listens for parent changes and for the current parent, saves its parent dependency.
     * During serialization EntityReplicator should make note of the dependency and send to clients.
     * On a client, it should wait until that dependency has been activated.
     */
    class NetworkHierarchyRootComponent final
        : public AZ::Component
        , public AZ::TransformNotificationBus::MultiHandler
    {
    public:
        AZ_COMPONENT(NetworkHierarchyRootComponent, "{2B124F70-B73A-43F2-A26E-D7B8C34E13B6}");

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
        void OnChildAdded(AZ::EntityId child) override;
        void OnChildRemoved(AZ::EntityId child) override;
        //! @}

        bool IsAttachedToAnotherHierarchy() const;

        const AZStd::vector<AZ::Entity*>& GetHierarchyChildren() const;

        void SetHierarchyRoot(NetworkHierarchyRootComponent* hierarchyRoot) { m_higherRoot = hierarchyRoot; }

        // maybe null if this is the highest root in the hierarchy
        // non-null if this is an attached root
        NetworkHierarchyRootComponent* GetHierarchyRoot() const { return m_higherRoot; }

    private:
        NetworkHierarchyRootComponent* m_higherRoot = nullptr;
        AZStd::vector<AZ::Entity*> m_children;
    };
}
