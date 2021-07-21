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
    template<typename ValueType, typename T>
    struct AnyTypeInfoConcept;

    namespace Serialization
    {
        template<class T, bool U, bool A>
        struct InstanceFactory;
    }
}

// Put this macro on your class to allow serialization with a private default constructor.
#define AZ_SERIALIZE_FRIEND() \
    template <typename, typename> \
    friend struct AZ::AnyTypeInfoConcept; \
    template <typename, bool, bool> \
    friend struct AZ::Serialization::InstanceFactory;