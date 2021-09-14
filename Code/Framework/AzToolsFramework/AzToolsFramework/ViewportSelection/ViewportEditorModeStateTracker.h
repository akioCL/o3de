/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/API/ViewportEditorModeStateTrackerNotificationBus.h>
#include <AzToolsFramework/API/ViewportEditorModeStateTrackerInterface.h>

namespace AzToolsFramework
{
    class EditorModeState
        : public EditorModeStateInterface
    {
    public:
        static constexpr AZ::u8 NumEditorModes = 4;


        void SetModeActive(EditorMode mode);

        void SetModeInactive(EditorMode mode);

        // EditorModeStateInterface ...
        bool IsModeActive(EditorMode mode) const override;
    private:
        AZStd::array<bool, NumEditorModes> m_editorModes{};
    };

    class ViewportEditorMode
        : public ViewportEditorModeInterface
    {
    public:
        void RegisterInterface();
        void UnregisterInterface();

        // ViewportEditorModeInterface ...
        void EnterMode(const ViewportEditorModeInfo& viewportEditorModeInfo, EditorMode mode) override;
        void ExitMode(const ViewportEditorModeInfo& viewportEditorModeInfo, EditorMode mode) override;
        const EditorModeStateInterface* GetEditorModeState(const ViewportEditorModeInfo& viewportEditorModeInfo) const override;
        size_t GetNumTrackedViewports() const override;
        bool IsViewportStateBeingTracked(const ViewportEditorModeInfo& viewportEditorModeInfo) const override;

    private:
        AZStd::unordered_map<typename ViewportEditorModeInfo::IdType, EditorModeState> m_viewportEditorModeStates;
    };
} // namespace AzToolsFramework
