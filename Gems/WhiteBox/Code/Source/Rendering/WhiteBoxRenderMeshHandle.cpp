/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxRenderMeshHandle.h"

#include <AzCore/std/createdestroy.h>

namespace WhiteBox
{
    WhiteBoxRenderMeshHandle::WhiteBoxRenderMeshHandle(const WhiteBoxRenderMeshHandle& other)
        : m_handle(other.m_handle)
    {
    }

    WhiteBoxRenderMeshHandle::WhiteBoxRenderMeshHandle(const WhiteBoxRenderMeshHandle&& other)
        : m_handle(AZStd::move(other.m_handle))
    {
    }

    WhiteBoxRenderMeshHandle& WhiteBoxRenderMeshHandle::operator=(const WhiteBoxRenderMeshHandle& other)
    {
        if (this != &other)
        {
            m_handle = other.m_handle;
        }

        return *this;
    }

    WhiteBoxRenderMeshHandle& WhiteBoxRenderMeshHandle::operator=(const WhiteBoxRenderMeshHandle&& other)
    {
        if (this != &other)
        {
            m_handle = AZStd::move(other.m_handle);
        }

        return *this;
    }
} // namespace WhiteBox
