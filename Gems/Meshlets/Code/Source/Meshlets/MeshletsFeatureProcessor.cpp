/*
* Modifications Copyright (c) Contributors to the Open 3D Engine Project. 
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
* 
* SPDX-License-Identifier: (Apache-2.0 OR MIT) AND MIT
*
*/

#include <AzCore/std/parallel/thread.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Transform.h>

#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

#include <Atom/Feature/RenderCommon.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessor.h>

#include <MeshletsRenderObject.h>
#include <MeshletsFeatureProcessor.h>

#ifndef SAFE_DELETE
#define SAFE_DELETE(p){if(p){delete p;p=nullptr;}}
#endif

namespace AZ
{
    namespace Meshlets
    {
        MeshletsFeatureProcessor::MeshletsFeatureProcessor()
        {
            MeshletsComputePassName = Name("MeshletsComputePass");
            MeshletsRenderPassName = Name("MeshletsRenderPass");
            CreateResources();
        }

        MeshletsFeatureProcessor::~MeshletsFeatureProcessor()
        {
            CleanResources();
        }

        void MeshletsFeatureProcessor::CreateResources()
        {
            if (!Meshlets::SharedBufferInterface::Get())
            {   // Since there can be several pipelines, allocate the shared buffer only for the
                // first one and from that moment on it will be used through its interface
                AZStd::string sharedBufferName = "MeshletsSharedBuffer";
                uint32_t bufferSize = 256 * 1024 * 1024;

                // Prepare Render Srg descriptors for calculating the required alignment for the shared buffer
                MeshRenderData tempRenderData;
                MeshletsRenderObject::PrepareRenderSrgDescriptors(tempRenderData, 1, 1);

                m_sharedBuffer = AZStd::make_unique<Meshlets::SharedBuffer>(sharedBufferName, bufferSize, tempRenderData.RenderBuffersDescriptors);
            }
        }

        void MeshletsFeatureProcessor::CleanResources()
        {
            m_sharedBuffer.reset();
        }

        void MeshletsFeatureProcessor::CleanPasses()
        {
            m_computePass = nullptr;
            m_renderPass = nullptr;
        }

        void MeshletsFeatureProcessor::Init([[maybe_unused]]RPI::RenderPipeline* pipeline)
        {
            InitComputePass(MeshletsComputePassName);
            InitRenderPass(MeshletsRenderPassName);
        }

        void MeshletsFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<MeshletsFeatureProcessor, RPI::FeatureProcessor>()
                    ->Version(0);
            }
        }

        void MeshletsFeatureProcessor::Activate()
        {
            m_transformServiceFeatureProcessor = GetParentScene()->GetFeatureProcessor<Render::TransformServiceFeatureProcessor>();
            AZ_Assert(m_transformServiceFeatureProcessor, "MeshFeatureProcessor requires a TransformServiceFeatureProcessor on its parent scene.");

            EnableSceneNotification();
            TickBus::Handler::BusConnect();
        }

        void MeshletsFeatureProcessor::Deactivate()
        {
            DisableSceneNotification();
            TickBus::Handler::BusDisconnect();
        }

        int MeshletsFeatureProcessor::GetTickOrder()
        {
            return AZ::TICK_PRE_RENDER;
        }

        bool MeshletsFeatureProcessor::HasMeshletPasses(RPI::RenderPipeline* renderPipeline)
        {
            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(MeshletsComputePassName, renderPipeline);
            RPI::Ptr<RPI::Pass> desiredPass = RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
            return desiredPass ? true : false;
        }

        bool MeshletsFeatureProcessor::InitComputePass(const Name& passName)
        {
            m_computePass = Data::Instance<MultiDispatchComputePass>();
            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(passName, m_renderPipeline);
            RPI::Ptr<RPI::Pass> desiredPass = RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);

            if (desiredPass)
            {
                m_computePass = static_cast<MultiDispatchComputePass*>(desiredPass.get());
                m_computeShader = m_computePass->GetShader();
            }
            else
            {
                AZ_Error("Meshlets", false,
                    "%s does not exist in this pipeline. Check your game project's .pass assets.",
                    passName.GetCStr());
                return false;
            }
            return true;
        }

        bool MeshletsFeatureProcessor::InitRenderPass(const Name& passName)
        {
            m_renderPass = Data::Instance<MeshletsRenderPass>();
            RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(passName, m_renderPipeline);
            RPI::Ptr<RPI::Pass> desiredPass = RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);

            if (desiredPass)
            {
                m_renderPass = static_cast<MeshletsRenderPass*>(desiredPass.get());
                m_renderShader = m_renderPass->GetShader();
            }
            else
            {
                AZ_Error("Meshlets", false,
                    "%s does not exist in this pipeline. Check your game project's .pass assets.",
                    passName.GetCStr());
                return false;
            }
            return true;
        }

        // This hook to the pipeline is still not connected and requires testing.
        // Current connection is by altering the two pipeline manually. Since the hook is
        // not the same for both pipelines, special care should be taken (on MainPipeline
        // it comes after OpaquePass while on the LowEndPipeline after MSAAResolvePass). It
        // is possible to simplify the logic and only hook to the active pipeline.
        bool MeshletsFeatureProcessor::AddMeshletsPassesToPipeline(RPI::RenderPipeline* renderPipeline)
        {
            const char* passRequestAssetFilePath = "Passes/MeshletsPassRequest.azasset";
            m_passRequestAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>(
                passRequestAssetFilePath, AZ::RPI::AssetUtils::TraceLevel::Warning);
            const AZ::RPI::PassRequest* passRequest = nullptr;
            if (m_passRequestAsset->IsReady())
            {
                passRequest = m_passRequestAsset->GetDataAs<AZ::RPI::PassRequest>();
            }
            if (!passRequest)
            {
                AZ_Error("Meshlets", false, "Failed to add meshlets pass. Can't load PassRequest from [%s]", passRequestAssetFilePath);
                return false;
            }

            // Create the pass
            RPI::Ptr<RPI::Pass> pass = RPI::PassSystemInterface::Get()->CreatePassFromRequest(passRequest);
            if (!pass)
            {
                AZ_Error("Meshlets", false, "Failed creating meshlets pass from pass request for pipeline [%s]",
                    renderPipeline->GetId().GetCStr());
                return false;
            }

            // Add the pass to render pipeline
            bool success = renderPipeline->AddPassAfter(pass, Name("OpaquePass"));

            AZ_Error("Meshlets", success, "Meshlets pass injection to render pipeline [%s] failed",
                renderPipeline->GetId().GetCStr());

            return success;
        }

        void MeshletsFeatureProcessor::SetTransform(
            const Render::TransformServiceFeatureProcessorInterface::ObjectId objectId, 
            const AZ::Transform& transform)
        {
            m_transformServiceFeatureProcessor->SetTransformForId(objectId, transform);
        }

        // Notice that currently this does not support object instancing.  Each object is assumed to
        // have a single ObjectId and PerObject srg.
        // To enhance this, create an ObjectInstanceData and per instance srg rather than per object
        // and create a new instance every time this method is invoked.
        // This will also require to split the srg from the srg with the meshlets buffers.
        Render::TransformServiceFeatureProcessorInterface::ObjectId
            MeshletsFeatureProcessor::AddMeshletsRenderObject(MeshletsRenderObject* meshletsRenderObject)
        {
            Render::TransformServiceFeatureProcessorInterface::ObjectId objectId = m_transformServiceFeatureProcessor->ReserveObjectId();

            if (m_renderPass)
            {
                m_renderPass->BuildDrawPacket(meshletsRenderObject->GetMeshletsRenderData(), objectId);
            }

            m_meshletsRenderObjects.push_back(meshletsRenderObject);

            AZ_Error("Meshlets", m_renderPass, "Meshlets object did not build DrawItem due to missing render pass");

            return objectId;
        }

        void MeshletsFeatureProcessor::RemoveMeshletsRenderObject(MeshletsRenderObject* meshletsRenderObject)
        {
            ModelLodDataArray& modelLodArray = meshletsRenderObject->GetMeshletsRenderData(0);
            for (auto& renderData : modelLodArray)
            {
                if (m_transformServiceFeatureProcessor && renderData)
                {
                    m_transformServiceFeatureProcessor->ReleaseObjectId(renderData->ObjectId);
                    break;  //same instance / object Id
                }
            }
            m_meshletsRenderObjects.remove(meshletsRenderObject);
        }

        void MeshletsFeatureProcessor::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            // OnTick can be used instead of the ::Simulate since it is set to be before the render
        }

        void MeshletsFeatureProcessor::Simulate(const RPI::FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_FUNCTION(AzRender);

            AZ_UNUSED(packet);
        }

        void MeshletsFeatureProcessor::Render([[maybe_unused]] const RPI::FeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_FUNCTION(AzRender);

            // Skip adding draw or dispatch items if there it no hair render objects
            if (m_meshletsRenderObjects.size() == 0)
            {
                return;
            }

            AZStd::list<RHI::DispatchItem*> dispatchItems;
            AZStd::list<const RHI::DrawPacket*> drawPackets;
            for (auto renderObject : m_meshletsRenderObjects)
            {
                // For demo purposed the model lod index is set for 0.
                // This entire control scheme will be removed to be replaced with GPU
                // driven pipeline control.
                ModelLodDataArray& modelLodArray = renderObject->GetMeshletsRenderData(0);

                for (auto& renderData : modelLodArray)
                {
                    if (renderData)
                    {
                        // the following is for testing only
                        Render::TransformServiceFeatureProcessorInterface::ObjectId objectId = renderData->ObjectId;
                        [[maybe_unused]] Transform transform = m_transformServiceFeatureProcessor->GetTransformForId(objectId);

                        if (MeshletsRenderObject::CreateAndBindComputeSrgAndDispatch(m_computeShader, *renderData))
                        {
                            dispatchItems.push_back(renderData->MeshDispatchItem.GetDispatchItem());
                        }
                        drawPackets.push_back(renderData->MeshDrawPacket);
                    }
                    AZ_Error("Meshlets", renderData, "Render data is NULL");
                }
            }
            m_computePass->AddDispatchItems(dispatchItems);
            m_renderPass->AddDrawPackets(drawPackets);
        }

        void MeshletsFeatureProcessor::OnRenderPipelinePassesChanged([[maybe_unused]] RPI::RenderPipeline* renderPipeline)
        {
            if (!HasMeshletPasses(renderPipeline))
            {   // This pipeline is not relevant - exist without changing anything.
                return;
            }

            m_renderPipeline = renderPipeline;
            CreateResources();

            // This should be used in the future instead of changing the actual pipeline file
//            AddMeshletsPassesToPipeline(m_renderPipeline);
            Init(m_renderPipeline);
        }

        void MeshletsFeatureProcessor::OnRenderPipelineAdded([[maybe_unused]] RPI::RenderPipelinePtr renderPipeline)
        {
            if (!HasMeshletPasses(renderPipeline.get()))
            {   // This pipeline is not relevant - exist without changing anything.
                return;
            }

            m_renderPipeline = renderPipeline.get();
            CreateResources();

            // This should be used in the future instead of changing the actual pipeline file
//            AddMeshletsPassesToPipeline(m_renderPipeline);
            Init(m_renderPipeline);
        }

        void MeshletsFeatureProcessor::OnRenderPipelineRemoved([[maybe_unused]] RPI::RenderPipeline* renderPipeline)
        {
            if (!HasMeshletPasses(renderPipeline))
            {   // This pipeline is not relevant - exist without changing anything.
                return;
            }

            m_renderPipeline = nullptr;
        }

    } // namespace AtomSceneStream
} // namespace AZ
