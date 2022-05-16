/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "RecastNavigationDebugDraw.h"
#include "RecastNavigationMeshConfig.h"

#include <DetourNavMesh.h>
#include <Recast.h>
#include <AzCore/Component/EntityId.h>
#include <Components/RecastHelpers.h>

namespace RecastNavigation
{
    //! Common navigation mesh logic for Recast navigation components. Recommended use is as a base class.
    class RecastNavigationMeshCommon
    {
    public:
        AZ_RTTI(RecastNavigationMeshCommon, "{D34CD5E0-8C29-4545-8734-9C7A92F03740}");
        virtual ~RecastNavigationMeshCommon() = default;
        
        //! Allocates and initializes Recast navigation mesh into @m_navMesh
        //! @param meshEntityId the entity's positions will be used as the center of the navigation mesh.
        //! @param tileSize the size of each square tile that form the navigation mesh. Recommended values are power of 2
        //! @return true if the navigation mesh object was successfully created
        bool CreateNavigationMesh(AZ::EntityId meshEntityId, float tileSize);

        //! Given a Recast data add a tile to the navigation mesh @m_navMesh
        //! @param navigationTileData the raw data of a Recast tile
        //! @return true if successful
        bool AttachNavigationTileToMesh(NavigationTileData& navigationTileData);

        //! Given a set of geometry and configuration create a Recast tile that can be attached using @AttachNavigationTileToMesh
        //! @param geom A set of geometry, triangle data
        //! @param meshConfig Recast navigation mesh configuration
        //! @param context Recast context object, @rcContext
        //! @return the tile data that can be attached to the navigation mesh using @AttachNavigationTileToMesh
        NavigationTileData CreateNavigationTile(TileGeometry* geom, const RecastNavigationMeshConfig& meshConfig, rcContext* context);

        //! Debug draw object for Recast navigation mesh.
        RecastNavigationDebugDraw m_customDebugDraw;

        //! Recast logging functionality and other optional tools.
        AZStd::unique_ptr<rcContext> m_context;

        //! Recast navigation mesh object.
        RecastPointer<dtNavMesh> m_navMesh;

        //! Recast navigation query object can be used to find paths.
        RecastPointer<dtNavMeshQuery> m_navQuery;
    };

} // namespace RecastNavigation
