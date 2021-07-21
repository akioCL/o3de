/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    namespace Serialization
    {
        /**
         * Interface for creating and destroying object from the serializer.
         */
        class IObjectFactory
        {
        public:

            virtual ~IObjectFactory() {}

            /// Called to create an instance of an object.
            virtual void* Create(const char* name) = 0;

            /// Called to destroy an instance of an object
            virtual void  Destroy(void* ptr) = 0;
            void Destroy(const void* ptr)
            {
                Destroy(const_cast<void*>(ptr));
            }
        };
    }
}
