/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/MeshComparison/MeshComparisonFeatureProcessorInterface.h>
#include <Atom/Utils/StableDynamicArray.h>
#include <MeshComparison/MeshComparisonInstance.h>

namespace AZ
{
    namespace Render
    {
        //! This feature processor handles static and dynamic non-skinned meshes.
        class MeshComparisonFeatureProcessor final
            : public MeshComparisonFeatureProcessorInterface
        {
        public:

            AZ_RTTI(AZ::Render::MeshComparisonFeatureProcessor, "{9312C0B9-9A50-4450-89A6-6FC1D963D796}", MeshComparisonFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            MeshComparisonFeatureProcessor() = default;
            virtual ~MeshComparisonFeatureProcessor() = default;

            // Render::MeshComparisonFeatureProcessorInterface overrides ...
            MeshComparisonHandle AcquireMeshComparison(
                const MeshComparisonDescriptor& descriptor) override;
                
            // FeatureProcessor overrides ...
            //! Creates pools, buffers, and buffer views
            void Activate() override;
            //! Releases GPU resources.
            void Deactivate() override;
            //! Updates GPU buffers with latest data from render proxies
            void Simulate(const FeatureProcessor::SimulatePacket& packet) override;

            // RPI::SceneNotificationBus overrides ...
            void OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline) override;
            void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline) override;

        private:
            MeshComparisonFeatureProcessor(const MeshComparisonFeatureProcessor&) = delete;

            void InitMeshComparisonPass(RPI::RenderPipeline* renderPipeline);
            Data::Asset<RPI::ModelAsset> CloneModelForComparison(
                const Data::Asset<RPI::ModelAsset>& modelAssetA, const Data::Asset<RPI::ModelAsset>& modelAssetB) const;

            StableDynamicArray<MeshComparisonInstance> m_meshComparisons;
        };
    } // namespace Render
} // namespace AZ
