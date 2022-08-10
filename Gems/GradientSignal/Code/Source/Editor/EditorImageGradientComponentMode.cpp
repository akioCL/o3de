/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include "EditorImageGradientComponentMode.h"

#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/Brushes/PaintBrushComponentNotificationBus.h>
#include <AzToolsFramework/Brushes/PaintBrushRequestBus.h>
#include <AzToolsFramework/Brushes/PaintBrushNotificationBus.h>
#include <AzToolsFramework/Manipulators/BrushManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/ImageGradientRequestBus.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

namespace GradientSignal
{
    EditorImageGradientComponentMode::EditorImageGradientComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
    {
        AzToolsFramework::PaintBrushNotificationBus::Handler::BusConnect(entityComponentIdPair);

        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldFromLocal, GetEntityId(), &AZ::TransformInterface::GetWorldTM);

        m_brushManipulator = AzToolsFramework::BrushManipulator::MakeShared(worldFromLocal, entityComponentIdPair);
        Refresh();

        m_brushManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);
    }

    EditorImageGradientComponentMode::~EditorImageGradientComponentMode()
    {
        AzToolsFramework::PaintBrushNotificationBus::Handler::BusDisconnect();
        m_brushManipulator->Unregister();

        AzToolsFramework::ScopedUndoBatch undo("Save Paint Layer");
        AzToolsFramework::PaintBrushComponentNotificationBus::Event(GetEntityComponentIdPair(), &AzToolsFramework::PaintBrushComponentNotificationBus::Events::SavePaintLayer);
        undo.MarkEntityDirty(GetEntityId());
    }

    AZStd::vector<AzToolsFramework::ActionOverride> EditorImageGradientComponentMode::PopulateActionsImpl()
    {
        // MAB TODO: FIXME - what should this return?
        return {};
    }


    bool EditorImageGradientComponentMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        bool result = false;

        AzToolsFramework::PaintBrushRequestBus::EventResult(
            result, GetEntityComponentIdPair(), &AzToolsFramework::PaintBrushRequestBus::Events::HandleMouseInteraction, mouseInteraction);
        return result;
    }

    void EditorImageGradientComponentMode::OnRadiusChanged(float radius)
    {
        m_brushManipulator->SetRadius(radius);
    }
    
    void EditorImageGradientComponentMode::OnWorldSpaceChanged(AZ::Transform result)
    {
        m_brushManipulator->SetSpace(result);
    }

    void EditorImageGradientComponentMode::OnPaint(const AZ::Aabb& dirtyArea)
    {
        AZ::Vector2 imagePixelsPerMeter(0.0f);
        ImageGradientRequestBus::EventResult(imagePixelsPerMeter, GetEntityId(), &ImageGradientRequestBus::Events::GetImagePixelsPerMeter);

        if ((imagePixelsPerMeter.GetX() <= 0.0f) || (imagePixelsPerMeter.GetY() <= 0.0f))
        {
            return;
        }

        const float xStep = 1.0f / imagePixelsPerMeter.GetX();
        const float yStep = 1.0f / imagePixelsPerMeter.GetY();

        const AZ::Vector3 minDistances = dirtyArea.GetMin();
        const AZ::Vector3 maxDistances = dirtyArea.GetMax();

        AZStd::vector<AZ::Vector3> points;

        for (float y = minDistances.GetY(); y <= maxDistances.GetY(); y += yStep)
        {
            for (float x = minDistances.GetX(); x <= maxDistances.GetX(); x += xStep)
            {
                points.emplace_back(x, y, minDistances.GetZ());
            }
        }

        AZStd::vector<float> intensities(points.size());
        AZStd::vector<float> opacities(points.size());
        AZStd::vector<bool> validFlags(points.size());

        AzToolsFramework::PaintBrushRequestBus::Event(
            GetEntityComponentIdPair(), &AzToolsFramework::PaintBrushRequestBus::Events::GetValues,
            points, intensities, opacities, validFlags);

        AZStd::vector<float> oldValues(points.size());
        GradientRequestBus::Event(GetEntityId(), &GradientRequestBus::Events::GetValues, points, oldValues);

        for (size_t index = 0; index < points.size(); index++)
        {
            if (validFlags[index])
            {
                float newValue = opacities[index] * intensities[index] + (1.0f - opacities[index]) * oldValues[index];
                GradientRequestBus::Event(GetEntityId(), &GradientRequestBus::Events::SetValue, points[index], newValue);
            }
        }

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
} // namespace GradientSignal
