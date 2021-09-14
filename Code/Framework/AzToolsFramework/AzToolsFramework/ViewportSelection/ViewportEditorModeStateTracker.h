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
    //! The encapsulation of the editor modes for a given viewport.
    class EditorModeState
        : public EditorModeStateInterface
    {
    public:

        //! The number of currently supported viewport editor modes.
        static constexpr AZ::u8 NumEditorModes = 4;

        //! Sets the specified mode as active.
        void SetModeActive(EditorMode mode);

        // Sets the specified mode as inactive.
        void SetModeInactive(EditorMode mode);

        // EditorModeStateInterface ...
        bool IsModeActive(EditorMode mode) const override;
    private:
        AZStd::array<bool, NumEditorModes> m_editorModes{}; //!< State flags to track active/inactive status of viewport editor modes.
    };

    //! The implementation of the central editor mode state tracker for all viewports.
    class ViewportEditorMode
        : public ViewportEditorModeInterface
    {
    public:
        //! Registers this object with the AZ::Interface.
        void RegisterInterface();

        //! Unregisters this object with the AZ::Interface.
        void UnregisterInterface();

        // ViewportEditorModeInterface ...
        void EnterMode(const ViewportEditorModeInfo& viewportEditorModeInfo, EditorMode mode) override;
        void ExitMode(const ViewportEditorModeInfo& viewportEditorModeInfo, EditorMode mode) override;
        const EditorModeStateInterface* GetEditorModeState(const ViewportEditorModeInfo& viewportEditorModeInfo) const override;
        size_t GetNumTrackedViewports() const override;
        bool IsViewportStateBeingTracked(const ViewportEditorModeInfo& viewportEditorModeInfo) const override;

    private:
        using ViewportEditorModeStates = AZStd::unordered_map<typename ViewportEditorModeInfo::IdType, EditorModeState>;
        ViewportEditorModeStates m_viewportEditorModeStates; //!< Editor mode state per viewport.
    };
} // namespace AzToolsFramework
