/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/API/EditorModeNotificationBus.h>

namespace AzToolsFramework
{
    /**
      * Provides a bus to track the editor mode state changes.
      */
    class EditorModeRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! Enters the specified editor mode.
        virtual void EnterMode([[maybe_unused]] EditorMode mode) {}

        //! Exits the specified editor mode.
        virtual void ExitMode([[maybe_unused]] EditorMode mode) {}
    };
    using EditorModeRequestsBus = AZ::EBus<EditorModeRequests>;

} // namespace AzToolsFramework

