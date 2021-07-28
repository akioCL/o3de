/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/std/containers/vector.h>
namespace LmbrCentral
{
    /// Buffers used for rendering Shapes.
    /// Generated from shape properties.
    struct ShapeMesh
    {
        AZStd::vector<AZ::Vector3> m_vertexBuffer; ///< Vertices of the shape.
        AZStd::vector<AZ::u32> m_indexBuffer; ///< Indices of the shape.
        AZStd::vector<AZ::Vector3> m_lineBuffer; ///< Lines of the shape.
    };

    /// Notifications sent by components that create meshes from shapes.
    class ShapeComponentMeshDataNotifications : public AZ::ComponentBus
    {
    public:
        //! allows multiple threads to call shape requests
        using MutexType = AZStd::recursive_mutex;

        /// @brief Informs listeners that the mesh data for the shape has changed
        virtual void OnShapeMeshDataChanged() = 0;
    };

    // Bus to service Shape mesh data notifications event group
    using ShapeComponentMeshDataNotificationBus = AZ::EBus<ShapeComponentMeshDataNotifications>;

    /// Services provided by components that create meshes from shapes.
    class ShapeComponentMeshDataRequests : public AZ::ComponentBus
    {
    public:
        /// allows multiple threads to call shape requests
        using MutexType = AZStd::recursive_mutex;

        /// @brief Returns the mesh data that can be used to render the shape.
        virtual const ShapeMesh* GetShapeMesh() const
        {
            AZ_Warning("ShapeComponentMeshDataRequests", false, "GetShapeMesh not implemented");
            return nullptr;
        }

        virtual ~ShapeComponentMeshDataRequests() = default;
    };

    // Bus to service the Shape component requests event group
    using ShapeComponentMeshDataRequestBus = AZ::EBus<ShapeComponentMeshDataRequests>;

} // namespace LmbrCentral
