/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/IDataSerializer.h>

namespace AZ
{
    IDataSerializerDeleter IDataSerializer::CreateDefaultDeleteDeleter()
    {
        return [](IDataSerializer* ptr)
        {
            delete ptr;
        };
    }
    IDataSerializerDeleter IDataSerializer::CreateNoDeleteDeleter()
    {
        return [](IDataSerializer*)
        {
        };
    }

}   // namespace AZ
