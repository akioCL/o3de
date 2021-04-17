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

#pragma once

#include <AzCore/EBus/EBus.h>

namespace RenderJoy
{
    class RenderJoyKeyboardNotifications
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        //////////////////////////////////////////////////////////////////////////

        struct KeyCodeInfo
        {
            uint32_t m_virtualScanCode; // bitwise AND with 0xff, and you'll get the JavaScript virtual key code used by ShaderToy.
            uint32_t m_nativeScanCode;
            int      m_qtKeyCode;
        };

        virtual void OnKeyDown(const KeyCodeInfo& keyCodeInfo) = 0;
        virtual void OnKeyUp(const KeyCodeInfo& keyCodeInfo) = 0;

    };

    using RenderJoyKeyboardNotificationBus = AZ::EBus<RenderJoyKeyboardNotifications>;

} // namespace RenderJoy
