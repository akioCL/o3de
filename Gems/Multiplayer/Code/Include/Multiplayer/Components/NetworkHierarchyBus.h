/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace Multiplayer
{
    class NetworkHierarchyNotifications
        : public AZ::ComponentBus
    {
    public:
        virtual void OnNetworkHierarchyUpdated([[maybe_unused]] const AZ::EntityId& rootEntityId) {}
        virtual void OnLeavingNetworkHierarchy() {}
    };

    typedef AZ::EBus<NetworkHierarchyNotifications> NetworkHierarchyNotificationBus;

    class NetworkHierarchyRequests
        : public AZ::ComponentBus
    {
    public:
        virtual const AZStd::vector<AZ::Entity*>& GetHierarchicalEntities() const = 0;
        virtual AZ::Entity* GetHierarchicalRoot() const = 0;
        virtual bool IsHierarchicalChild() const = 0;
        virtual bool IsHierarchicalRoot() const = 0;
    };

    typedef AZ::EBus<NetworkHierarchyRequests> NetworkHierarchyRequestBus;
}
