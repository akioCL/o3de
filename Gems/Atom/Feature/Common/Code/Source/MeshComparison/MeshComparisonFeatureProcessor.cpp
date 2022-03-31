/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MeshComparison/MeshComparisonFeatureProcessor.h>

#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelLodAssetCreator.h>

namespace AZ
{
    namespace Render
    {
        static AZ::Data::Asset<AZ::RPI::BufferAsset> CreateBufferAsset(
        const void* data, const size_t elementCount, AZ::RHI::Format format)
    {
        AZ::RPI::BufferAssetCreator creator;

        AZ::Data::AssetId assetId;
        assetId.m_guid = AZ::Uuid::CreateRandom();
        creator.Begin(assetId);

        AZ::RHI::BufferViewDescriptor bufferViewDescriptor =
            AZ::RHI::BufferViewDescriptor::CreateTyped(0, static_cast<uint32_t>(elementCount), format);

        AZ::RHI::BufferDescriptor bufferDescriptor;
        bufferDescriptor.m_bindFlags = AZ::RHI::BufferBindFlags::InputAssembly | AZ::RHI::BufferBindFlags::ShaderRead;
        bufferDescriptor.m_byteCount = bufferViewDescriptor.m_elementSize * bufferViewDescriptor.m_elementCount;

        creator.SetBuffer(data, data ? bufferDescriptor.m_byteCount : 0, bufferDescriptor);

        creator.SetBufferViewDescriptor(bufferViewDescriptor);

        creator.SetUseCommonPool(AZ::RPI::CommonBufferPoolType::DynamicInputAssembly);

        AZ::Data::Asset<AZ::RPI::BufferAsset> bufferAsset;
        creator.End(bufferAsset);

        return bufferAsset;
    }

        void MeshComparisonFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<MeshComparisonFeatureProcessor, FeatureProcessor>()
                    ->Version(0);
            }
        }

        void MeshComparisonFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "MeshComparisonFeatureProcessor: Simulate");

            [[maybe_unused]] AZ::Job* parentJob = packet.m_parentJob;
        }

        MeshComparisonFeatureProcessorInterface::MeshComparisonHandle MeshComparisonFeatureProcessor::AcquireMeshComparison(
            const MeshComparisonDescriptor& descriptor)
        {
            // Don't need to check the concurrency during emplace() because the StableDynamicArray won't move the other elements during insertion
            MeshComparisonHandle meshComparisonHandle = m_meshComparisons.emplace();
            
            // Set the model assets that are being compared
            meshComparisonHandle->m_modelAssetA = descriptor.m_modelAssetA;
            meshComparisonHandle->m_modelAssetB = descriptor.m_modelAssetB;
            meshComparisonHandle->m_comparisonModelAsset =
                CloneModelForComparison(meshComparisonHandle->m_modelAssetA, meshComparisonHandle->m_modelAssetB);
            meshComparisonHandle->m_comparisonModel = RPI::Model::FindOrCreate(meshComparisonHandle->m_comparisonModelAsset);
            // The caller must handle the callback event in order to get the model with the new data stream
            AZ_Assert(descriptor.m_meshComparisonCompleteHandler, "Must call MeshComparisonFeatureProcessor::AcquireMeshComparison with a valid MeshComparisonCompleteEvent::Handler");
            descriptor.m_meshComparisonCompleteHandler->Connect(meshComparisonHandle->m_meshComparisonCompleteEvent);

            // Because we're doing all the work right here for now, just go ahead and fire off the event
            meshComparisonHandle->m_meshComparisonCompleteEvent.Signal(meshComparisonHandle->m_comparisonModel);

            return meshComparisonHandle;
        }

        void MeshComparisonFeatureProcessor::Activate()
        {
            EnableSceneNotification();
        }

        void MeshComparisonFeatureProcessor::Deactivate()
        {
            DisableSceneNotification();
        }

        void MeshComparisonFeatureProcessor::OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline)
        {
            InitMeshComparisonPass(pipeline.get());
        }

        void MeshComparisonFeatureProcessor::OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline)
        {
            InitMeshComparisonPass(renderPipeline);
        }

        void MeshComparisonFeatureProcessor::InitMeshComparisonPass(RPI::RenderPipeline* renderPipeline)
        {
            RPI::PassFilter meshComparisonPassFilter = RPI::PassFilter::CreateWithPassName(AZ::Name{ "MeshComparisonPass" }, renderPipeline);
            RPI::Ptr<RPI::Pass> meshComparisonPass = RPI::PassSystemInterface::Get()->FindFirstPass(meshComparisonPassFilter);
            if (meshComparisonPass)
            {
                //SkinnedMeshComputePass* skinnedMeshComputePass = azdynamic_cast<SkinnedMeshComputePass*>(skinningPass.get());
                //skinnedMeshComputePass->SetFeatureProcessor(this);

                // There may be multiple skinning passes in the scene due to multiple pipelines, but there is only one skinning shader
                //m_skinningShader = skinnedMeshComputePass->GetShader();

                //if (!m_skinningShader)
                //{
                //    AZ_Error(s_featureProcessorName, false, "Failed to get skinning pass shader. It may need to finish processing.");
                //}
                //else
                //{
                //    m_cachedSkinningShaderOptions.SetShader(m_skinningShader);
                //}
            }
        }

        Data::Asset<RPI::ModelAsset> MeshComparisonFeatureProcessor::CloneModelForComparison(
            const Data::Asset<RPI::ModelAsset>& modelAssetA, const Data::Asset<RPI::ModelAsset>& modelAssetB) const
        {
            // First determine which model has more vertices. This will be the one we use for the distance calculation
            Data::Asset<RPI::ModelAsset> largeModelAsset;
            if (modelAssetA->GetLodAssets()[0]->GetMeshes()[0].GetIndexBufferAssetView().GetBufferAsset()->GetBufferViewDescriptor().m_elementCount >=
                modelAssetB->GetLodAssets()[0]->GetMeshes()[0].GetIndexBufferAssetView().GetBufferAsset()->GetBufferViewDescriptor().m_elementCount)
            {
                largeModelAsset = modelAssetA;
            }
            else
            {
                largeModelAsset = modelAssetB;
            }

            // Now clone it, but with an additional vertex stream for distance
            // Only lod0 is supported for now
            Data::Asset<RPI::ModelLodAsset> largeLodAsset = largeModelAsset->GetLodAssets()[0];

            // Create a unique id so it doesn't conflict with the original asset
            RPI::ModelAssetCreator modelCreator;
            modelCreator.Begin(Uuid::CreateRandom());

            modelCreator.SetName(AZStd::string("MeshComparison_%s", largeModelAsset->GetName().GetCStr()));


            //
            // Lod
            //
            RPI::ModelLodAssetCreator modelLodCreator;
            modelLodCreator.Begin(Data::AssetId(Uuid::CreateRandom()));      

            // Add the lod buffers
            const auto& mesh0 = largeLodAsset->GetMeshes()[0];
            modelLodCreator.AddLodStreamBuffer(mesh0.GetIndexBufferAssetView().GetBufferAsset());
            modelLodCreator.AddLodStreamBuffer(mesh0.GetSemanticBufferAssetView(Name{"POSITION"})->GetBufferAsset());

            // Create and add the new distance buffer
            // Vertex count for the lod is equal to the number of positions
            uint32_t lodVertexCount = mesh0.GetSemanticBufferAssetView(Name{"POSITION"})->GetBufferAsset()->GetBufferViewDescriptor().m_elementCount;

            RHI::Format distanceType = RHI::Format::R32_FLOAT;
            Data::Asset<RPI::BufferAsset> distanceBufferAsset = CreateBufferAsset(nullptr, lodVertexCount, distanceType);
            modelLodCreator.AddLodStreamBuffer(distanceBufferAsset);
            //
            // Submesh
            //
            uint32_t meshVertexOffset = 0;
            for (const auto& mesh : largeLodAsset->GetMeshes())
            {
                uint32_t meshVertexCount = mesh.GetVertexCount();
                modelLodCreator.BeginMesh();

                // Set the index and position buffer views
                modelLodCreator.SetMeshIndexBuffer(mesh.GetIndexBufferAssetView());
                modelLodCreator.AddMeshStreamBuffer(
                    RHI::ShaderSemantic{ "POSITION" }, AZ::Name(), *mesh.GetSemanticBufferAssetView(Name{"POSITION"}));

                // Add the view to the new distance buffer
                RHI::BufferViewDescriptor distanceBufferDescriptor =
                    RHI::BufferViewDescriptor::CreateTyped(meshVertexOffset, meshVertexCount, distanceType);
                
                modelLodCreator.AddMeshStreamBuffer(
                    RHI::ShaderSemantic{ "DISTANCE" }, AZ::Name(),
                    AZ::RPI::BufferAssetView{ distanceBufferAsset, distanceBufferDescriptor });

                AZ::Aabb localAabb = mesh.GetAabb();
                modelLodCreator.SetMeshAabb(AZStd::move(localAabb));
                modelLodCreator.SetMeshMaterialSlot(mesh.GetMaterialSlotId());

                modelLodCreator.EndMesh();

                meshVertexOffset += meshVertexCount;
            }
            

            Data::Asset<RPI::ModelLodAsset> lodAsset;
            modelLodCreator.End(lodAsset);
            modelCreator.AddLodAsset(AZStd::move(lodAsset));

            Data::Asset<RPI::ModelAsset> modelAsset;
            modelCreator.End(modelAsset);

            return modelAsset;
        }

    } // namespace Render
} // namespace AZ
