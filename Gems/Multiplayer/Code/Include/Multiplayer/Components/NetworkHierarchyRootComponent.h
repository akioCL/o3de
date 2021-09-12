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
    //! @brief Component that declares the top level entity of a network hierarchy.
    /*
     * Call @GetHierarchyChildren to get the list of hierarchical entities.
     * A network hierarchy is meant to be a small group of entities. You can control the maximum supported size of
     * a network hierarchy by modifying CVar @bg_hierarchyEntityMaxLimit.
     *
     * A root component marks either a top most root of a hierarchy, or an inner root of an attach hierarchy.
     */
    class NetworkHierarchyRootComponent final
        : public AZ::Component
        , protected AZ::TransformNotificationBus::MultiHandler
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

        //! Children are guaranteed to have either @NetworkHierarchyChildComponent or @NetworkHierarchyRootComponent
        //! @returns all hierarchical children
        const AZStd::vector<AZ::Entity*>& GetHierarchyChildren() const;

        //! @returns the highest level root of the hierarchy if this root isn't one, nullptr otherwise.
        AZ::Entity* GetTopLevelHierarchyRoot() const { return m_higherRoot; }

        //! @returns true if this is an inner root, use @GetTopLevelHierarchyRoot to get top level root of the hierarchy.
        bool IsAttachedToAnotherHierarchy() const;

    protected:
        //! AZ::TransformNotificationBus::Handler overrides.
        //! @{
        void OnParentChanged(AZ::EntityId oldParent, AZ::EntityId newParent) override;
        void OnChildAdded(AZ::EntityId child) override;
        void OnChildRemoved(AZ::EntityId childRemovedId) override;
        //! @}

        void SetTopLevelHierarchyRoot(AZ::Entity* hierarchyRoot) { m_higherRoot = hierarchyRoot; }

    private:
        AZ::Entity* m_higherRoot = nullptr;
        AZStd::vector<AZ::Entity*> m_children;
        
        //! @returns false if the maximum supported hierarchy size has been reached
        bool RecursiveAttachHierarchicalEntities(AZ::EntityId underEntity, uint32_t& currentEntityCount);
        //! @returns false if the maximum supported hierarchy size has been reached
        bool RecursiveAttachHierarchicalChild(AZ::EntityId entity, uint32_t& currentEntityCount);
    };
}
