#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneCore/DataTypes/IManifestObject.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            void IManifestObject::Reflect(AZ::ReflectContext* context)
            {
                if(AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<IManifestObject>()
                        ->Version(0);
                }
            }
        }  //namespace DataTypes
    }  //namespace SceneAPI
}  //namespace AZ
