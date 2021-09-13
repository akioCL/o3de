/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/EditorModeNotificationBus.h>
#include <AzToolsFramework/ViewportSelection/ViewportEditorMode.h>

namespace AzToolsFramework
{
    static constexpr const char* ViewportEditorModeLogWindow = "ViewportEditorMode";

    void EditorModeState::SetModeActive(EditorMode mode)
    {
        if (const AZ::u32 modeIndex = static_cast<AZ::u32>(mode);
            modeIndex < NumEditorModes)
        {
            m_editorModes[modeIndex] = true;
        }
        else
        {
            AZ_Error(ViewportEditorModeLogWindow, false, "Cannot activate mode %u, mode is not recognized", modeIndex)
        }
    }

    void EditorModeState::SetModeInactive(EditorMode mode)
    {
        if (const AZ::u32 modeIndex = static_cast<AZ::u32>(mode); modeIndex < NumEditorModes)
        {
            m_editorModes[modeIndex] = false;
        }
        else
        {
            AZ_Error(ViewportEditorModeLogWindow, false, "Cannot deactivate mode %u, mode is not recognized", modeIndex)
        }
    }

    bool EditorModeState::IsModeActive(EditorMode mode) const
    {
        return m_editorModes[static_cast<AZ::u32>(mode)];
    }

    void ViewportEditorMode::RegisterInterface()
    {
        ViewportEditorModeInterfaceWrapper::Register(this);
    }

    void ViewportEditorMode::UnregisterInterface()
    {
        if (ViewportEditorModeInterfaceWrapper::Get() != nullptr)
        {
            ViewportEditorModeInterfaceWrapper::Unregister(this);
        }
    }

    void ViewportEditorMode::EnterMode(const ViewportEditorModeInfo& viewportEditorModeInfo, EditorMode mode)
    {
        auto& editorModeStates = m_viewportEditorModeStates[viewportEditorModeInfo.m_id];
        if (editorModeStates.IsModeActive(mode))
        {
            AZ_Warning(
                ViewportEditorModeLogWindow, false,
                AZStd::string::format(
                    "Duplicate call to EnterMode for mode '%u' on id '%i'", static_cast<AZ::u32>(mode), viewportEditorModeInfo.m_id).c_str());
        }
        editorModeStates.SetModeActive(mode);
        EditorModeNotificationsBus::Event(
            viewportEditorModeInfo.m_id, &EditorModeNotificationsBus::Events::OnEditorModeEnter, editorModeStates, mode);
    }

    void ViewportEditorMode::ExitMode(const ViewportEditorModeInfo& viewportEditorModeInfo, EditorMode mode)
    {
        EditorModeState* editorModeStates = nullptr;
        if (m_viewportEditorModeStates.count(viewportEditorModeInfo.m_id))
        {
            editorModeStates = &m_viewportEditorModeStates.at(viewportEditorModeInfo.m_id);
            AZ_Warning(
                ViewportEditorModeLogWindow, editorModeStates->IsModeActive(mode),
                AZStd::string::format(
                    "Duplicate call to ExitMode for mode '%u' on id '%i'", static_cast<AZ::u32>(mode), viewportEditorModeInfo.m_id).c_str());
        }
        else
        {
            AZ_Warning(
                ViewportEditorModeLogWindow, false, "Call to ExitMode for mode '%u' on id '%i' without precursor call to EnterMode",
                static_cast<AZ::u32>(mode), viewportEditorModeInfo.m_id);

            editorModeStates = &m_viewportEditorModeStates[viewportEditorModeInfo.m_id];
        }

        editorModeStates->SetModeInactive(mode);
        EditorModeNotificationsBus::Event(
            viewportEditorModeInfo.m_id, &EditorModeNotificationsBus::Events::OnEditorModeExit, *editorModeStates, mode);
    }

    const EditorModeStateInterface* ViewportEditorMode::GetEditorModeState(const ViewportEditorModeInfo& viewportEditorModeInfo) const
    {
        if (auto editorModeStates = m_viewportEditorModeStates.find(viewportEditorModeInfo.m_id);
            editorModeStates != m_viewportEditorModeStates.end())
        {
            return &editorModeStates->second;
        }
        else
        {
            return nullptr;
        }
    }

    size_t ViewportEditorMode::GetNumTrackedViewports() const
    {
        return m_viewportEditorModeStates.size();
    }

    bool ViewportEditorMode::IsViewportStateBeingTracked(const ViewportEditorModeInfo& viewportEditorModeInfo) const
    {
        return m_viewportEditorModeStates.count(viewportEditorModeInfo.m_id) > 0;
    }
} // namespace AzToolsFramework
