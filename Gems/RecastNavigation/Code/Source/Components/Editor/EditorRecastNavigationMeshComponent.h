/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/EBus/ScheduledEvent.h>
#include <AzCore/Task/TaskGraph.h>
#include <Components/RecastNavigationMeshCommon.h>
#include <Components/RecastNavigationMeshConfig.h>
#include <ToolsComponents/EditorComponentBase.h>

namespace RecastNavigation
{
    //! Editor version of @RecastNavigationMeshComponent.
    class EditorRecastNavigationMeshComponent final
        : public AzToolsFramework::Components::EditorComponentBase
        , public RecastNavigationMeshCommon
    {
    public:
        AZ_EDITOR_COMPONENT(EditorRecastNavigationMeshComponent, "{22D516D4-C98D-4783-85A4-1ABE23CAB4D4}", AzToolsFramework::Components::EditorComponentBase);
        EditorRecastNavigationMeshComponent();
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        //! EditorComponentBase interface implementation
        //! @{
        void Activate() override;
        void Deactivate() override;
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        //! @}

        void OnDebugDrawTick();
        void OnUpdateEvent();

    private:
        RecastNavigationMeshConfig m_meshConfig;
        bool m_enableDebugDraw = false;
        AZ::ScheduledEvent m_debugDrawEvent;
        void OnDebugDrawChanged();

        bool m_enableAutoUpdateInEditor = false;
        AZ::ScheduledEvent m_updateEvent;
        void OnAutoUpdateChanged();

        void CreateEditorNavigationMesh();
        AZStd::atomic<bool> m_isUpdating{ false };

        friend class EditorNavigationTest;
    };
} // namespace RecastNavigation
