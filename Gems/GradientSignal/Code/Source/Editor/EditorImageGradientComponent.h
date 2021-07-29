/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ComponentMode/ComponentModeDelegate.h>
#include <AzToolsFramework/Brushes/PaintBrush.h>
#include <AzToolsFramework/Brushes/PaintBrushComponentNotificationBus.h>
#include <Components/ImageGradientComponent.h>
#include <GradientSignal/Editor/EditorGradientComponentBase.h>

#include <Atom/RHI.Reflect/Format.h>

namespace GradientSignal
{
    class EditorImageGradientComponent
        : public EditorGradientComponentBase<ImageGradientComponent, ImageGradientConfig>
        , private AZ::Data::AssetBus::Handler
        , private AzToolsFramework::PaintBrushComponentNotificationBus::Handler
    {
    public:
        using BaseClassType = EditorGradientComponentBase<ImageGradientComponent, ImageGradientConfig>;
        AZ_EDITOR_COMPONENT(EditorImageGradientComponent, EditorImageGradientComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        void Activate() override;
        void Deactivate() override;
        
        void SavePaintLayer() override;
        AZ::RHI::Format GetFormat(const AZ::Data::Asset<ImageAsset>& imageAsset);
        void WriteOutputFile(const AZStd::string& filePath);

        void OnCompositionChanged() override;

        static constexpr const char* const s_categoryName = "Gradients";
        static constexpr const char* const s_componentName = "Image Gradient";
        static constexpr const char* const s_componentDescription = "Generates a gradient by sampling an image asset";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Gradient.svg";        
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Gradient.svg";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/";

    protected:
        using ComponentModeDelegate = AzToolsFramework::ComponentModeFramework::ComponentModeDelegate;
        ComponentModeDelegate m_componentModeDelegate;
        
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        bool InComponentMode() const;
        AZ::Crc32 InOverrideMode() const;

    private:
        AzToolsFramework::PaintBrush m_paintBrush;

        AZStd::string m_path;
    };
}
