/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Component/ComponentBus.h>

namespace RecastNavigation
{
    class RecastNavigationSurveyorRequests
        : public AZ::ComponentBus
    {
    public:
        //! Collects and adds the geometry within the defined world space.
        //! @param geometryData an out parameter that will have the geometry added to.
        virtual void CollectGeometry(BoundedGeometry& geometryData) = 0;
        
        //! Returns the world bounds that this surveyor is responsible for.
        //! @return An axis aligned bounding box of the world bounds.
        virtual AZ::Aabb GetWorldBounds() = 0;
    };

    using RecastNavigationSurveyorRequestBus = AZ::EBus<RecastNavigationSurveyorRequests>;

} // namespace RecastNavigation
