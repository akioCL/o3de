/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MeshComparison/MeshComparisonDispatchItem.h>

#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Model/ModelLod.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/RPIUtils.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI.Reflect/ShaderInputNameIndex.h>

#include <AzCore/Math/PackedVector3.h>

namespace AZ
{
    namespace Render
    {
        static RHI::Ptr<RHI::BufferView> CreateBufferViewFromBufferAssetView(const RPI::BufferAssetView& bufferAssetView)
        {
            Data::Instance<RPI::Buffer> buffer = RPI::Buffer::FindOrCreate(bufferAssetView.GetBufferAsset());
            if (buffer)
            {
                AZ::RHI::Ptr<AZ::RHI::BufferView> bufferView = RHI::Factory::Get().CreateBufferView();
                {
                    bufferView->SetName(Name("MeshComparisonBufferView"));
                    [[maybe_unused]] RHI::ResultCode resultCode = bufferView->Init(*buffer->GetRHIBuffer(), bufferAssetView.GetBufferViewDescriptor());
                    AZ_Error("MeshComparisonBufferView", resultCode == RHI::ResultCode::Success, "Failed to initialize buffer view for mesh comparison.");
                }

                return bufferView;
            }
            return nullptr;
        }

        MeshComparisonDispatchItem::MeshComparisonDispatchItem(
            const Data::Asset<RPI::ModelAsset>& modelAssetA,
            const Data::Asset<RPI::ModelAsset>& modelAssetB,
            const Data::Asset<RPI::ModelAsset>& comparisonModelAsset)
            : m_modelAssetA(modelAssetA)
            , m_modelAssetB(modelAssetB)
            , m_comparisonModelAsset(comparisonModelAsset)
        {
        }

        MeshComparisonDispatchItem::~MeshComparisonDispatchItem()
        {

        }

        bool MeshComparisonDispatchItem::Init()
        {
            m_modelA = RPI::Model::FindOrCreate(m_modelAssetA);
            m_modelB = RPI::Model::FindOrCreate(m_modelAssetB);
            m_comparisonModel = RPI::Model::FindOrCreate(m_comparisonModelAsset);

            // Create a dispatch item for each vertex of the largest model.
            // Each dispatch item will have a total thread count equal to the number of triangles in the smaller model
            // Each thread will find the distance from the vertex in the larger model to the nearest triangle in the smaller model
            // Thus the inputs are (positions only for now)
            // The position of the larger mesh
            // The index buffer of the smaller mesh
            // The positions of the smaller mesh
            // The triangle count of the smaller mesh

            // The outputs are:
            // The distance to each triangle in the smaller model

            // A secondary pass will create another dispatch item for each vertex to iterate over all
            // of the distances for any given vertex, find the smallest one,
            // and write it to a vertex buffer in the comparison model
            // so that it can be rendered using a debug shader

            // Only lod0 is supported
            Data::Asset<RPI::ModelLodAsset> largeLodAsset = m_modelAssetA->GetLodAssets()[0];
            Data::Asset<RPI::ModelLodAsset> smallLodAsset = m_modelAssetB->GetLodAssets()[0];
            Data::Asset<RPI::ModelLodAsset> comparisonModelLodAsset = m_comparisonModelAsset->GetLodAssets()[0];
            Data::Instance<RPI::ModelLod> largeLod = m_modelA->GetLods()[0];
            Data::Instance<RPI::ModelLod> smallLod = m_modelB->GetLods()[0];

            // Make sure we're dealing with the correct large vs small meshes
            if (largeLodAsset->GetMeshes()[0].GetIndexBufferAssetView().GetBufferAsset()->GetBufferViewDescriptor().m_elementCount <
                smallLodAsset->GetMeshes()[0].GetIndexBufferAssetView().GetBufferAsset()->GetBufferViewDescriptor().m_elementCount)
            {
                AZStd::swap(largeLodAsset, smallLodAsset);
                AZStd::swap(largeLod, smallLod);
            }

            // Get a view into the output buffer
            Data::Asset<RPI::BufferAsset> distanceBufferAsset = comparisonModelLodAsset->GetMeshes()[0].GetSemanticBufferAssetView(Name{"DISTANCE"})->GetBufferAsset();
            Data::Instance<RPI::Buffer> distanceBuffer = RPI::Buffer::FindOrCreate(distanceBufferAsset);

            uint32_t largeMeshCount = aznumeric_caster(largeLod->GetMeshes().size());
            m_positionBufferViews.reserve(largeMeshCount);
            m_indexBufferViews.reserve(largeMeshCount);
            m_triangleDistanceDispatchItems.reserve(largeMeshCount);
            m_triangleDistanceInstanceSrgs.reserve(largeMeshCount);
            uint32_t lodVertexIndex = 0;
            for (uint32_t largeMeshIndex = 0; largeMeshIndex < largeMeshCount; ++largeMeshIndex)
            {
                // Get the mesh
                const RPI::ModelLodAsset::Mesh& largeLodAssetMesh = largeLodAsset->GetMeshes()[largeMeshIndex];
                uint32_t vertexCount = largeLodAssetMesh.GetVertexCount();

                // Get the buffer holding the original positions
                const auto& originalPositionBuffer = largeLodAssetMesh.GetSemanticBufferTyped<PackedVector3f>(Name{"POSITION"});

                for (uint32_t meshVertexIndex = 0; meshVertexIndex < vertexCount; ++meshVertexIndex)
                {                    
                    AZ::PackedVector3f position = originalPositionBuffer[meshVertexIndex];
                //
                // Get the position stream
                //
                /*RPI::ShaderInputContract inputContract;
                RPI::ShaderInputContract::StreamChannelInfo channelInfo;
                channelInfo.m_semantic = RHI::ShaderSemantic{ Name{ "POSITION" } };
                channelInfo.m_isOptional = false;
                channelInfo.m_componentCount = RHI::GetFormatComponentCount(RHI::Format::R32G32B32_FLOAT);
                inputContract.m_streamChannels.push_back(channelInfo);

                RHI::InputStreamLayout inputLayout;
                RPI::ModelLod::StreamBufferViewList streamBufferViews;
                largeLod->GetStreamsForMesh(inputLayout, streamBufferViews, nullptr, inputContract, largeMeshIndex);

                if (streamBufferViews.empty())
                {
                    return false;
                }

                const RHI::StreamBufferView& streamBufferView = streamBufferViews[0];
                AZ::RHI::Ptr<AZ::RHI::BufferView> positionBufferView;
                if (streamBufferView.GetByteCount() > 0)
                {                    
                    uint32_t elementOffset = streamBufferView.GetByteOffset() / streamBufferView.GetByteStride();
                    uint32_t elementCount = streamBufferView.GetByteCount() / streamBufferView.GetByteStride();

                    // 3-component float buffers are not supported on metal for non-input assembly buffer views,
                    // so use a float view instead
                    RHI::BufferViewDescriptor descriptor = RHI::BufferViewDescriptor::CreateTyped(elementOffset * 3, elementCount * 3, RHI::Format::R32_FLOAT);

                    positionBufferView = RHI::Factory::Get().CreateBufferView();
                    {
                        // Initialize the buffer view
                        AZStd::string bufferViewName = AZStd::string::format(
                            "MeshComparisonInput_%s_mesh%" PRIu32 "", largeLodAsset->GetName().GetCStr(), largeMeshIndex);
                        positionBufferView->SetName(Name(bufferViewName));
                        RHI::ResultCode resultCode = positionBufferView->Init(*streamBufferView.GetBuffer(), descriptor);

                        if (resultCode != RHI::ResultCode::Success)
                        {
                            AZ_Error("MeshComparisonDispatchItem", false, "Failed to initialize buffer view %s.", bufferViewName.c_str());
                            return false;
                        }
                    }

                    // Hold a reference to the view so it doesn't get released
                    m_positionBufferViews.push_back(positionBufferView);
                }*/

                    for (const auto& smallMesh : smallLodAsset->GetMeshes())
                    {
                        // We're going to go ahead and KISS and just use the entire lod buffer view for the small mesh lod
                        const RPI::BufferAssetView& indexBufferAssetView = smallMesh.GetIndexBufferAssetView();
                        const RPI::BufferAssetView* positionBufferAssetView = smallMesh.GetSemanticBufferAssetView(Name{"POSITION"});
                        uint32_t smallMeshTriangleCount = indexBufferAssetView.GetBufferViewDescriptor().m_elementCount / 3;

                        //
                        // Create the instance srg
                        //

                        if (!m_triangleDistanceShader)
                        {
                            AZ_Error("MeshComparisonDispatchItem", false, "Cannot initialize a MeshComparisonDispatchItem with a null shader");
                            return false;
                        }

                        // Get the shader variant and instance SRG
                        // No shader options for now
                        RPI::ShaderOptionGroup shaderOptionGroup = m_triangleDistanceShader->CreateShaderOptionGroup();
                        shaderOptionGroup.SetUnspecifiedToDefaultValues();
                        const RPI::ShaderVariant& shaderVariant = m_triangleDistanceShader->GetVariant(shaderOptionGroup.GetShaderVariantId());

                        RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptor;
                        shaderVariant.ConfigurePipelineState(pipelineStateDescriptor);

                        auto perInstanceSrgLayout = m_triangleDistanceShader->FindShaderResourceGroupLayout(AZ::Name{ "InstanceSrg" });
                        if (!perInstanceSrgLayout)
                        {
                            AZ_Error("MeshComparisonDispatchItem", false, "Failed to get shader resource group layout");
                            return false;
                        }

                        Data::Instance<RPI::ShaderResourceGroup> instanceSrg = RPI::ShaderResourceGroup::Create(
                            m_triangleDistanceShader->GetAsset(), m_triangleDistanceShader->GetSupervariantIndex(), perInstanceSrgLayout->GetName());

                        if (!instanceSrg)
                        {
                            AZ_Error("MeshComparisonDispatchItem", false, "Failed to create shader resource group for mesh comparison");
                            return false;
                        }

                        RHI::ShaderInputConstantIndex positionIndex =
                            instanceSrg->FindShaderInputConstantIndex(Name{ "m_position" });
                        if (!positionIndex.IsValid())
                        {
                            AZ_Error(
                                "MeshComparisonDispatchItem", false,
                                "Failed to find shader input index for m_position in the mesh comparison compute shader per-instance SRG.");
                            return false;
                        }
                        AZ::Vector3 fourComponentPosition = AZ::Vector3(position.GetX(), position.GetY(), position.GetZ());
                        instanceSrg->SetConstant(positionIndex, fourComponentPosition);

                        RHI::ShaderInputNameIndex indexBufferNameIndex = "m_indexBuffer";
                        RHI::Ptr<RHI::BufferView> indexBufferView = CreateBufferViewFromBufferAssetView(indexBufferAssetView);
                        instanceSrg->SetBufferView(indexBufferNameIndex, indexBufferView.get());
                        m_indexBufferViews.push_back(indexBufferView);

                        
                        RHI::ShaderInputNameIndex positionBufferNameIndex = "m_positionBuffer";
                        RHI::Ptr<RHI::BufferView> positionBufferView = CreateBufferViewFromBufferAssetView(*positionBufferAssetView);
                        instanceSrg->SetBufferView(positionBufferNameIndex, positionBufferView.get());
                        m_positionBufferViews.push_back(positionBufferView);


                        
                        //
                        // Set the output buffer view and index into that view
                        //
                        RHI::ShaderInputConstantIndex outputVertexIndexIndex =
                            instanceSrg->FindShaderInputConstantIndex(Name{ "m_outputVertexIndex" });
                        if (!outputVertexIndexIndex.IsValid())
                        {
                            AZ_Error(
                                "MeshComparisonDispatchItem", false,
                                "Failed to find shader input index for m_outputVertexIndex in the mesh comparison compute shader per-instance SRG.");
                            return false;
                        }
                        instanceSrg->SetConstant(outputVertexIndexIndex, lodVertexIndex);
                        
                        RHI::ShaderInputNameIndex distanceBufferNameIndex = "m_outputDistanceBuffer";
                        instanceSrg->SetBuffer(distanceBufferNameIndex, distanceBuffer);


                        m_triangleDistanceInstanceSrgs.push_back(instanceSrg);

                        //
                        // Set the buffer views on the instance srg
                        //

                        //
                        // Create the dispatch item
                        //
                        RHI::DispatchItem distanceDispatchItem;

                        distanceDispatchItem.m_uniqueShaderResourceGroup = instanceSrg->GetRHIShaderResourceGroup();
                        distanceDispatchItem.m_pipelineState = m_triangleDistanceShader->AcquirePipelineState(pipelineStateDescriptor);

                        auto& arguments = distanceDispatchItem.m_arguments.m_direct;
                        const auto outcome = RPI::GetComputeShaderNumThreads(m_triangleDistanceShader->GetAsset(), arguments);
                        if (!outcome.IsSuccess())
                        {
                            AZ_Error("SkinnedMeshInputBuffers", false, outcome.GetError().c_str());
                        }

                        arguments.m_totalNumberOfThreadsX = smallMeshTriangleCount;
                        arguments.m_totalNumberOfThreadsY = 1;
                        arguments.m_totalNumberOfThreadsZ = 1;

                        m_triangleDistanceDispatchItems.push_back(distanceDispatchItem);
                    }

                    ++lodVertexIndex;
                }
            }

            return true;
        }
    } // namespace Render
} // namespace AZ
