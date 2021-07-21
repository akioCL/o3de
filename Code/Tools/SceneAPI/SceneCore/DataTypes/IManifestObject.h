#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IManifestObject
            {
            public:
                AZ_RTTI(IManifestObject, "{3B839407-1884-4FF4-ABEA-CA9D347E83F7}");
                static void Reflect(AZ::ReflectContext* context);

                virtual ~IManifestObject() = default;
                virtual void OnUserAdded() {};
                virtual void OnUserRemoved() const {};
            };

        }  //namespace DataTypes
    }  //namespace SceneAPI
}  //namespace AZ
