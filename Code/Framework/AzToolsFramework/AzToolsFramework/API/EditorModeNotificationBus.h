/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

namespace AzToolsFramework
{
    enum class EditorMode : AZ::u8
    {
        None,
        Component,
        Focus
    };

    //! Provides a bus to notify when the different editor modes are entered/exit.
    class EditorModeNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! Notifies the entering of the specified editor mode.
        virtual void OnEditorModeEnter([[maybe_unused]] EditorMode mode) {}

        //! Notifies the exiting of the specified editor mode.
        virtual void OnEditorModeExit([[maybe_unused]] EditorMode mode) {}
    };
    using EditorModeNotificationsBus = AZ::EBus<EditorModeNotifications>;
}
