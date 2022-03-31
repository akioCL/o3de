/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/MeshComparison/MeshComparisonFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>

namespace AZ
{
    namespace Render
    {
        //! The data for a single mesh comparison operation
        class MeshComparisonInstance
        {
        public:
            AZ_RTTI(AZ::Render::MeshComparisonInstance, "{EBC35B86-B396-42AD-B44A-4D43A60A3267}");
            virtual ~MeshComparisonInstance();

            Data::Asset<RPI::ModelAsset> m_modelAssetA;
            Data::Asset<RPI::ModelAsset> m_modelAssetB;
            Data::Asset<RPI::ModelAsset> m_comparisonModelAsset;
            Data::Instance<RPI::Model> m_modelA;
            Data::Instance<RPI::Model> m_modelB;
            Data::Instance<RPI::Model> m_comparisonModel;
            MeshComparisonCompleteEvent m_meshComparisonCompleteEvent;
        };
    } // namespace Render
} // namespace AZ
