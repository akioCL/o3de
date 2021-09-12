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
     * The parent of this entity should have @NetworkHierarchyChildComponent (or @NetworkHierarchyRootComponent).
     * A network hierarchy is a collection of entities with one @NetworkHierarchyRootComponent at the top parent
     * and one or more @NetworkHierarchyChildComponent on its child entities.
     */
    class NetworkHierarchyChildComponent final
        : public AZ::Component
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

        void SetHierarchyRoot(NetworkHierarchyRootComponent* hierarchyRoot);
        //! @returns top most @NetworkHierarchyRootComponent for the hierarchy
        NetworkHierarchyRootComponent* GetHierarchyRoot() const;

    private:
        NetworkHierarchyRootComponent* m_hierarchyRoot = nullptr;
    };
}
