/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/Trace.h>

namespace WhiteBox
{
    //! Abstract handle for the rendering API-specific handle to the whitebox mesh.
    //! @note There are no type checks to ensure the handle type returned is the same as the handle type stored.
    class WhiteBoxRenderMeshHandle
    {
    public:
        WhiteBoxRenderMeshHandle() = default;

        WhiteBoxRenderMeshHandle(const WhiteBoxRenderMeshHandle& other);
        WhiteBoxRenderMeshHandle(const WhiteBoxRenderMeshHandle&& other);
        WhiteBoxRenderMeshHandle& operator=(const WhiteBoxRenderMeshHandle& other);
        WhiteBoxRenderMeshHandle& operator=(const WhiteBoxRenderMeshHandle&& other);

        //! Accepts the rendering API-specific mesh handle pointer and abstracts it away as a void pointer.
        template<typename T>
        WhiteBoxRenderMeshHandle(const T* handle)
            : m_handle(static_cast<const void*>(handle))
        {
        }

        template<typename T>
        WhiteBoxRenderMeshHandle& operator=(const T* other)
        {
            m_handle = static_cast<const void*>(other);
            return *this;
        }

        //! Takes the abstracted mesh handle and casts it to the rendering API-specific mesh handle pointer.
        template<typename T>
        const T* GetAs() const
        {
            return static_cast<const T*>(m_handle);
        }

    private:
        //! The rendering API-specific mesh handle pointer abstracted away as a void pointer.
        const void* m_handle = nullptr;
    };
} // namespace WhiteBox
