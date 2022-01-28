/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>

namespace Camera
{
    AZ_PUSH_DISABLE_WARNING_GCC("-Wunused-variable")
    const static char* s_viewportCameraSelectorName = "Viewport Camera Selector";
    AZ_POP_DISABLE_WARNING_GCC
    extern void RegisterViewportCameraSelectorWindow();
} // namespace Camera
