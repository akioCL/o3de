/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/EditorModeFeedback/EditorEditorModeFeedbackSystemComponent.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Model/ModelLodUtils.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawInterface.h>
#include <Atom/Utils/Utils.h>

namespace AZ
{
    namespace Render
    {
        //! Builds the mesh draw packets for the specified model.
        static AZStd::vector<RPI::MeshDrawPacket> BuildMeshDrawPackets(
            const RPI::View* view,
            const AZ::Transform& worldTM,
            const Data::Asset<RPI::ModelAsset>& modelAsset,
            const Data::Instance<RPI::Material>& material,
            const Data::Instance<RPI::ShaderResourceGroup>& meshObjectSrg)
        {
            AZStd::vector<RPI::MeshDrawPacket> meshDrawPackets;
            const RPI::Model& model = *RPI::Model::FindOrCreate(modelAsset);
            const auto modelLodIndex = RPI::ModelLodUtils::SelectLod(view, worldTM, model);
            const Data::Asset<RPI::ModelLodAsset>& modelLodAsset = modelAsset->GetLodAssets()[0];
            RPI::ModelLod& modelLod = *RPI::ModelLod::FindOrCreate(modelLodAsset, modelAsset).get();
        
            for (auto i = 0; i < modelLod.GetMeshes().size(); i++)
            {
                RPI::MeshDrawPacket drawPacket(modelLod, i, material, meshObjectSrg);
                meshDrawPackets.push_back(AZStd::move(drawPacket));
            }
        
            return meshDrawPackets;
        }

        //! Creates the mask shader resource group for the specified mesh.
        static Data::Instance<RPI::ShaderResourceGroup> CreateMaskShaderResourceGroup(
            const MeshFeatureProcessorInterface* featureProcessor,
            const MeshFeatureProcessorInterface::MeshHandle& meshHandle,
            Data::Instance<RPI::Material> maskMaterial)
        {
            const auto& shaderAsset = maskMaterial->GetAsset()->GetMaterialTypeAsset()->GetShaderAssetForObjectSrg();
            const auto& objectSrgLayout = maskMaterial->GetAsset()->GetObjectSrgLayout();
            const auto maskMeshObjectSrg = RPI::ShaderResourceGroup::Create(shaderAsset, objectSrgLayout->GetName());

            // Set the object id so the correct MVP matrices can be selected in the shader
            const auto objectId = featureProcessor->GetObjectId(meshHandle).GetIndex();
            RHI::ShaderInputNameIndex objectIdIndex = "m_objectId";
            maskMeshObjectSrg->SetConstant(objectIdIndex, objectId);

            //! Set the id to write to the entity mask texture.
            RHI::ShaderInputNameIndex maskIdIndex = "m_maskId";
            maskMeshObjectSrg->SetConstant(maskIdIndex, 1);
            
            maskMeshObjectSrg->Compile();

            return maskMeshObjectSrg;
        }

        //! Gets the view for the specified scene.
        static const RPI::ViewPtr GetViewFromScene(const RPI::Scene* scene)
        {
            const auto viewportContextRequests = RPI::ViewportContextRequests::Get();
            const auto viewportContext = viewportContextRequests->GetViewportContextByScene(scene);
            const RPI::ViewPtr viewPtr = viewportContext->GetDefaultView();
            return viewPtr;
        }

        //! Creates the material for the mask pass shader.
        static Data::Instance<RPI::Material> CreateMaskMaterial()
        {
            const AZStd::string path = "shaders/postprocessing/editormodemask.azmaterial";
            const auto materialAsset = GetAssetFromPath<RPI::MaterialAsset>(path, Data::AssetLoadBehavior::PreLoad, true);
            const auto maskMaterial = RPI::Material::FindOrCreate(materialAsset);
            return maskMaterial;
        }

        //! Get the world transform for the specified entity.
        static AZ::Transform GetWorldTransformForEntity(EntityId entityId)
        {
            AZ::Transform worldTM;
            AZ::TransformBus::EventResult(worldTM, entityId, &AZ::TransformBus::Events::GetWorldTM);
            return worldTM;
        }

        EditorEditorModeFeedbackSystemComponent::MeshHandleDrawPackets::~MeshHandleDrawPackets() = default;

        void EditorEditorModeFeedbackSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<EditorEditorModeFeedbackSystemComponent, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<EditorEditorModeFeedbackSystemComponent>(
                        "Editor Mode Feedback System", "Manages discovery of Editr Mode Feedback effects")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }

        EditorEditorModeFeedbackSystemComponent::~EditorEditorModeFeedbackSystemComponent() = default;

        void EditorEditorModeFeedbackSystemComponent::Activate()
        {
            AzToolsFramework::Components::EditorComponentBase::Activate();
            AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusConnect(AzToolsFramework::GetEntityContextId());
            AZ::Interface<EditorModeFeedbackInterface>::Register(this);
            AZ::TickBus::Handler::BusConnect();
        }

        void EditorEditorModeFeedbackSystemComponent::Deactivate()
        {
            AZ::TickBus::Handler::BusDisconnect();
            AZ::Interface<EditorModeFeedbackInterface>::Unregister(this);
            AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
            AzToolsFramework::Components::EditorComponentBase::Deactivate();
        }

        bool EditorEditorModeFeedbackSystemComponent::IsEnabled() const
        {
            return m_enabled;
        }

        void EditorEditorModeFeedbackSystemComponent::RegisterDrawableComponent(
            EntityComponentIdPair entityComponentId, const MeshFeatureProcessorInterface::MeshHandle& meshHandle)
        {
            // Overwrite any existing mesh handle for this component-entity id
            auto& meshHandleDrawPackets =
                m_entityComponentMeshHandleDrawPackets[entityComponentId.GetEntityId()][entityComponentId.GetComponentId()];
            meshHandleDrawPackets.m_meshHandle = &meshHandle;

            // The same component can call RegisterDrawableEntity multiple times in order to update its model asset so always
            // clear any existing draw packets for the component upon registration
            meshHandleDrawPackets.m_meshDrawPackets.clear();
        }

        void EditorEditorModeFeedbackSystemComponent::OnEditorModeActivated(
            [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState,
            AzToolsFramework::ViewportEditorMode mode)
        {
            // Purge the draw packets for all registered 
            if (mode == AzToolsFramework::ViewportEditorMode::Focus)
            {
                for (auto& [entityId, componentMeshHandleDrawPackets] : m_entityComponentMeshHandleDrawPackets)
                {
                    for (auto& [componentid, meshHandleDrawPackets] : componentMeshHandleDrawPackets)
                    {
                        meshHandleDrawPackets.m_meshDrawPackets.clear();
                    }
                }

                m_enabled = true;
            }
        }

        void EditorEditorModeFeedbackSystemComponent::OnEditorModeDeactivated(
            [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState,
            AzToolsFramework::ViewportEditorMode mode)
        {
            if (mode == AzToolsFramework::ViewportEditorMode::Focus)
            {
                m_enabled = false;
            }
        }

        void EditorEditorModeFeedbackSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            if (!m_enabled)
            {
                return;
            }

            const auto focusModeInterface = AZ::Interface<AzToolsFramework::FocusModeInterface>::Get();
            if (!focusModeInterface)
            {
                return;
            }

            // TODO: see if there is a more reliable method of creating the required resources once the depender systems are initialized
            if (!m_maskMaterial)
            {
                m_maskMaterial = CreateMaskMaterial();
            }

            // Build the draw packets (where required) for each registered component and add them to the draw list
            const auto focusedEntityIds = focusModeInterface->GetFocusedEntities(AzToolsFramework::GetEntityContextId());
            for (const auto& focusedEntityId : focusedEntityIds)
            {
                const auto entityComponentMeshHandleDrawPacketsIterator = m_entityComponentMeshHandleDrawPackets.find(focusedEntityId);
                if (entityComponentMeshHandleDrawPacketsIterator == m_entityComponentMeshHandleDrawPackets.end())
                {
                    // No drawable data for this entity
                    continue;
                }

                auto& [entityId, componentMeshHandleDrawPackets] = *entityComponentMeshHandleDrawPacketsIterator;

                for (auto& [componentId, meshHandleDrawPackets] : componentMeshHandleDrawPackets)
                {
                    const auto scene = RPI::Scene::GetSceneForEntityId(entityId);
                    if (meshHandleDrawPackets.m_meshDrawPackets.empty())
                    {
                        if (const auto featureProcessor =
                                RPI::Scene::GetFeatureProcessorForEntity<MeshFeatureProcessorInterface>(focusedEntityId))
                        {
                            const auto maskMeshObjectSrg =
                                CreateMaskShaderResourceGroup(featureProcessor, *meshHandleDrawPackets.m_meshHandle, m_maskMaterial);
                            const auto view = GetViewFromScene(scene);
                            const auto worldTM = GetWorldTransformForEntity(focusedEntityId);
                            meshHandleDrawPackets.m_meshDrawPackets = BuildMeshDrawPackets(
                                view.get(),
                                worldTM,
                                featureProcessor->GetModelAsset(*meshHandleDrawPackets.m_meshHandle),
                                m_maskMaterial,
                                maskMeshObjectSrg);
                        }
                        else
                        {
                            // This really shouldn't fail, but just in case...
                            AZ_Error(
                                "EditorEditorModeFeedbackSystemComponent",
                                false,
                                AZStd::string::format("Could't get feature processor for entity '%s'", focusedEntityId.ToString().c_str()).c_str());
                        }
                    }

                    AZ::RPI::DynamicDrawInterface* dynamicDraw = AZ::RPI::GetDynamicDraw();
                    for (auto& drawPacket : meshHandleDrawPackets.m_meshDrawPackets)
                    {
                        drawPacket.Update(*scene);
                        dynamicDraw->AddDrawPacket(scene, drawPacket.GetRHIDrawPacket());
                    }
                }
            }
        }

        int EditorEditorModeFeedbackSystemComponent::GetTickOrder()
        {
            return AZ::TICK_PRE_RENDER;
        }
    } // namespace Render
} // namespace AZ
