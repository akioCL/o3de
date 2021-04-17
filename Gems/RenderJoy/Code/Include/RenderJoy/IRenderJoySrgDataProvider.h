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

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Math/Vector2.h>

namespace RenderJoy
{
    //! This is the AZ::Interface<> declaration for the singleton responsible
    //! for providing per frame data for RenderJoy shaders.
    class IRenderJoySrgDataProvider
    {
    public:
        AZ_TYPE_INFO(IRenderJoySrgDataProvider, "{A7C2F151-4A15-4B17-B932-D966BE4BB6C5}");

        static constexpr const char* LogName = "IRenderJoySrgDataProvider";

        virtual ~IRenderJoySrgDataProvider() = default;

        virtual float GetTime() = 0;
        virtual float GetTimeDelta() = 0;
        virtual int GetFramesCount() = 0;
        virtual float GetFramesPerSecond() = 0;
        virtual void GetMouseData(AZ::Vector2& currentPos, AZ::Vector2& clickPos, bool& isLeftButtonDown, bool& isLeftButtonClick) = 0;
        virtual void ResetFrameCounter(int newValue) = 0;
    };

} // namespace RenderJoy
