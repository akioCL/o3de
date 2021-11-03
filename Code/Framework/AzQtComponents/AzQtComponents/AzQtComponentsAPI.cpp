/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzCore/Module/Environment.h>

namespace AzQtComponents
{
    AZ_QT_COMPONENTS_API void InitializeDynamicModule(void* env)
    {
        AZ::Environment::Attach(static_cast<AZ::EnvironmentInstance>(env));
    }

    AZ_QT_COMPONENTS_API void UninitializeDynamicModule()
    {
        AZ::Environment::Detach();
    }
} // namespace AzQtComponents
