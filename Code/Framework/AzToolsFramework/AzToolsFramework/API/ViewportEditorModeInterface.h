/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/API/EditorModeNotificationBus.h>

namespace AzToolsFramework
{
    class ViewportEditorModeInterface
    {
    public:
        AZ_RTTI(ViewportEditorModeInterface, "{7D72A4F7-2147-4ED9-A315-E456A3BE3CF6}");

        //! Enters the specified editor mode.
        virtual void EnterMode(const ViewportEditorModeInfo& viewportEditorModeInfo, EditorMode mode) = 0;
       
        //! Exits the specified editor mode.
        virtual void ExitMode(const ViewportEditorModeInfo& viewportEditorModeInfo, EditorMode mode) = 0;

        virtual const EditorModeStateInterface* GetEditorModeState(const ViewportEditorModeInfo& viewportEditorModeInfo) const = 0;

        virtual size_t GetNumTrackedViewports() const = 0;

        virtual bool IsViewportStateBeingTracked(const ViewportEditorModeInfo& viewportEditorModeInfo) const = 0;

    private:
    };

    using ViewportEditorModeInterfaceWrapper = AZ::Interface<ViewportEditorModeInterface>;

} // namespace AzToolsFramework

