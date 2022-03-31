/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/Utils/StableDynamicArray.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/EBus/Event.h>

namespace AZ
{
    namespace RPI
    {
        class ModelAsset;
        class Model;
    }

    namespace Render
    {
        class MeshComparisonInstance;

        using MeshComparisonCompleteEvent = Event<const Data::Instance<RPI::Model>>;
        //! Mesh comparison settings and the callback handler for when the comparison is complete
        struct MeshComparisonDescriptor
        {
            Data::Asset<RPI::ModelAsset> m_modelAssetA;
            Data::Asset<RPI::ModelAsset> m_modelAssetB;
            MeshComparisonCompleteEvent::Handler* m_meshComparisonCompleteHandler;
        };

        //! MeshComparisonFeatureProcessorInterface provides an interface to initiate a mesh comparison request from the underlying MeshComparisonFeatureProcessorInterface
        class MeshComparisonFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::MeshFeatureProcessorInterface, "{1F072CF1-DD34-432F-909E-986CB38D07AA}", FeatureProcessor);

            using MeshComparisonHandle = StableDynamicArrayHandle<MeshComparisonInstance>;

            //! Creates a new ModelAsset and Model with an extra vertex stream containing the difference between the two models
            virtual MeshComparisonHandle AcquireMeshComparison(
                const MeshComparisonDescriptor& descriptor) = 0;
        };
    } // namespace Render
} // namespace AZ
