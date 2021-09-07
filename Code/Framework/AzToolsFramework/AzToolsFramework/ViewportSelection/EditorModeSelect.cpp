/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ViewportSelection/EditorModeSelect.h>

namespace AzToolsFramework
{
    EditorModeSelect::EditorModeSelect()
    {
        EditorModeRequestsBus::Handler::BusConnect();
    }

    EditorModeSelect::~EditorModeSelect()
    {
        EditorModeRequestsBus::Handler::BusDisconnect();
    }

    void EditorModeSelect::EnterMode(EditorMode mode)
    {
        m_editorMode = mode;
        AZ_Printf("LYN-5265", "EnterMode(%u)", static_cast<AZ::u8>(mode));
        AzToolsFramework::EditorModeNotificationsBus::Broadcast(
            &AzToolsFramework::EditorModeNotificationsBus::Events::OnEditorModeEnter, EditorMode::Component);
    }

    void EditorModeSelect::ExitMode(EditorMode mode)
    {
        AZ_Printf("LYN-5265", "ExitMode(%u)", static_cast<AZ::u8>(mode));
        AzToolsFramework::EditorModeNotificationsBus::Broadcast(
            &AzToolsFramework::EditorModeNotificationsBus::Events::OnEditorModeExit, EditorMode::Component);
        m_editorMode = EditorMode::Undefined;
    }
} // namespace AzToolsFramework
