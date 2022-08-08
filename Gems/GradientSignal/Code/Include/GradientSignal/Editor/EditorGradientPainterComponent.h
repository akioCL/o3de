/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/Path/Path.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EntityTypes.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <GradientSignal/Ebuses/GradientPreviewContextRequestBus.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Editor/EditorGradientImageCreatorRequestBus.h>
#include <GradientSignal/Editor/EditorGradientTypeIds.h>
#include <GradientSignal/GradientSampler.h>

#include <LmbrCentral/Dependency/DependencyMonitor.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

namespace GradientSignal
{
    class GradientPainterConfig : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(GradientPainterConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(GradientPainterConfig, "{324D408C-2118-42CA-90BC-53DC3E5CF8A4}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        AZ::EntityId m_inputBounds;
        AZ::Vector2 m_outputResolution = AZ::Vector2(512.0f);
        OutputFormat m_outputFormat = OutputFormat::R32;
        AZ::IO::Path m_outputImagePath;
    };

    class EditorGradientPainterComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , private GradientRequestBus::Handler
        , private GradientImageCreatorRequestBus::Handler
        , private GradientPreviewContextRequestBus::Handler
        , private LmbrCentral::DependencyNotificationBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(
            EditorGradientPainterComponent, EditorGradientPainterComponentTypeId, AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);

        EditorGradientPainterComponent() = default;

        //! Component overrides ...
        void Activate() override;
        void Deactivate() override;

        //! GradientRequestBus overrides ...
        float GetValue(const GradientSampleParams& sampleParams) const override;
        void GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const override;
        bool IsEntityInHierarchy(const AZ::EntityId& entityId) const override;

        //! GradientImageCreatorRequestBus overrides ...
        AZ::EntityId GetInputBounds() const override;
        void SetInputBounds(const AZ::EntityId& inputBounds) override;
        AZ::Vector2 GetOutputResolution() const override;
        void SetOutputResolution(const AZ::Vector2& resolution) override;
        OutputFormat GetOutputFormat() const override;
        void SetOutputFormat(OutputFormat outputFormat) override;
        AZ::IO::Path GetOutputImagePath() const override;
        void SetOutputImagePath(const AZ::IO::Path& outputImagePath) override;

        //! LmbrCentral::DependencyNotificationBus overrides ...
        void OnCompositionChanged() override;

        static constexpr const char* const s_categoryName = "Gradients";
        static constexpr const char* const s_componentName = "Gradient Painter";
        static constexpr const char* const s_componentDescription = "Provides in-Editor painting into a streaming image asset";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Gradient.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Gradient.svg";
        static constexpr const char* const s_helpUrl = "";

    protected:
        //! AzToolsFramework::EntitySelectionEvents overrides ...
        void OnSelected() override;
        void OnDeselected() override;

        //! GradientPreviewContextRequestBus overrides ...
        AZ::EntityId GetPreviewEntity() const override;
        AZ::Aabb GetPreviewBounds() const override;

        void OnConfigurationChanged();

        // This is used by the preview so we can pass an invalid entity Id if our component is disabled
        AZ::EntityId GetGradientEntityId() const;

        void UpdatePreviewSettings() const;
        AzToolsFramework::EntityIdList CancelPreviewRendering() const;

        void SetupDependencyMonitor();

    private:
        GradientPainterConfig m_configuration;
        AZ::EntityId m_gradientEntityId;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
    };
} // namespace GradientSignal
