/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <API/EditorAssetSystemAPI.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpMaterialImporter.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneBuilder/Importers/ImporterUtilities.h>
#include <SceneAPI/SceneBuilder/Importers/Utilities/AssImpMeshImporterUtilities.h>
#include <SceneAPI/SceneBuilder/Importers/Utilities/RenamedNodesMap.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpMaterialWrapper.h>
#include <SceneAPI/SceneData/GraphData/MaterialData.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Events/ProcessingResult.h>
#include <assimp/scene.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            AssImpMaterialImporter::AssImpMaterialImporter()
            {
                BindToCall(&AssImpMaterialImporter::ImportMaterials);
            }

            void AssImpMaterialImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<AssImpMaterialImporter, SceneCore::LoadingComponent>()->Version(1);
                }
            }
            
            SceneData::GraphData::MaterialData::TextureFlags ToSceneApiTextureFlags(AZ::u32 assTextureFlags)
            {
                return static_cast<SceneData::GraphData::MaterialData::TextureFlags>(assTextureFlags);
            }

            Events::ProcessingResult AssImpMaterialImporter::ImportMaterials(AssImpSceneNodeAppendedContext& context)
            {
                AZ_TraceContext("Importer", "Material");
                if (!context.m_sourceNode.ContainsMesh())
                {
                    return Events::ProcessingResult::Ignored;
                }

                Events::ProcessingResultCombiner combinedMaterialImportResults;

                AZStd::unordered_map<int, AZStd::shared_ptr<SceneData::GraphData::MaterialData>> materialMap;
                for (unsigned int idx = 0; idx < context.m_sourceNode.GetAssImpNode()->mNumMeshes; ++idx)
                {
                    int meshIndex = context.m_sourceNode.GetAssImpNode()->mMeshes[idx];
                    const aiMesh* assImpMesh = context.m_sourceScene.GetAssImpScene()->mMeshes[meshIndex];
                    AZ_Assert(assImpMesh, "Asset Importer Mesh should not be null.");
                    int materialIndex = assImpMesh->mMaterialIndex;
                    AZ_TraceContext("Material Index", materialIndex);

                    auto matFound = materialMap.find(materialIndex);

                    AZStd::shared_ptr<SceneData::GraphData::MaterialData> material;
                    AZStd::string materialName;

                    if (matFound == materialMap.end())
                    {
                        std::shared_ptr<AssImpSDKWrapper::AssImpMaterialWrapper> assImpMaterial =
                            std::shared_ptr<AssImpSDKWrapper::AssImpMaterialWrapper>(new AssImpSDKWrapper::AssImpMaterialWrapper(context.m_sourceScene.GetAssImpScene()->mMaterials[materialIndex]));

                        materialName = assImpMaterial->GetName().c_str();
                        RenamedNodesMap::SanitizeNodeName(materialName, context.m_scene.GetGraph(), context.m_currentGraphPosition, "Material");
                        AZ_TraceContext("Material Name", materialName);

                        material = AZStd::make_shared<SceneData::GraphData::MaterialData>();

                        auto setTexturePathAndFlags = [&](DataTypes::IMaterialData::TextureMapType sceneApiMapType, SDKMaterial::MaterialWrapper::MaterialMapType assMapType)
                        {
                            material->SetTexture(sceneApiMapType,
                                ResolveTexturePath(context.m_sourceScene.GetSceneFileName(), assImpMaterial->GetTextureFileName(assMapType)).c_str());
                            material->SetTextureFlags(sceneApiMapType,
                                ToSceneApiTextureFlags(assImpMaterial->GetTextureFlags(assMapType)));
                        };

                        material->SetMaterialName(assImpMaterial->GetName());
                        setTexturePathAndFlags(DataTypes::IMaterialData::TextureMapType::Diffuse, SDKMaterial::MaterialWrapper::MaterialMapType::Diffuse);
                        setTexturePathAndFlags(DataTypes::IMaterialData::TextureMapType::Specular, SDKMaterial::MaterialWrapper::MaterialMapType::Specular);
                        setTexturePathAndFlags(DataTypes::IMaterialData::TextureMapType::Bump, SDKMaterial::MaterialWrapper::MaterialMapType::Bump);
                        setTexturePathAndFlags(DataTypes::IMaterialData::TextureMapType::Normal, SDKMaterial::MaterialWrapper::MaterialMapType::Normal);
                        material->SetUniqueId(assImpMaterial->GetUniqueId());
                        material->SetDiffuseColor(assImpMaterial->GetDiffuseColor());
                        material->SetSpecularColor(assImpMaterial->GetSpecularColor());
                        material->SetEmissiveColor(assImpMaterial->GetEmissiveColor());
                        material->SetShininess(assImpMaterial->GetShininess());

                        material->SetUseColorMap(assImpMaterial->GetUseColorMap());
                        material->SetBaseColor(assImpMaterial->GetBaseColor());

                        material->SetUseMetallicMap(assImpMaterial->GetUseMetallicMap());
                        material->SetMetallicFactor(assImpMaterial->GetMetallicFactor());
                        setTexturePathAndFlags(DataTypes::IMaterialData::TextureMapType::Metallic, SDKMaterial::MaterialWrapper::MaterialMapType::Metallic);

                        material->SetUseRoughnessMap(assImpMaterial->GetUseRoughnessMap());
                        material->SetRoughnessFactor(assImpMaterial->GetRoughnessFactor());
                        setTexturePathAndFlags(DataTypes::IMaterialData::TextureMapType::Roughness, SDKMaterial::MaterialWrapper::MaterialMapType::Roughness);

                        material->SetUseEmissiveMap(assImpMaterial->GetUseEmissiveMap());
                        material->SetEmissiveIntensity(assImpMaterial->GetEmissiveIntensity());

                        material->SetUseAOMap(assImpMaterial->GetUseAOMap());
                        setTexturePathAndFlags(DataTypes::IMaterialData::TextureMapType::AmbientOcclusion, SDKMaterial::MaterialWrapper::MaterialMapType::AmbientOcclusion);
                        
                        setTexturePathAndFlags(DataTypes::IMaterialData::TextureMapType::Emissive, SDKMaterial::MaterialWrapper::MaterialMapType::Emissive);
                        
                        setTexturePathAndFlags(DataTypes::IMaterialData::TextureMapType::BaseColor, SDKMaterial::MaterialWrapper::MaterialMapType::BaseColor);

                        material->SetTwoSided(assImpMaterial->GetTwoSided());

                        AZ_Assert(material, "Failed to allocate scene material data.");
                        if (!material)
                        {
                            combinedMaterialImportResults += Events::ProcessingResult::Failure;
                            continue;
                        }

                        materialMap[materialIndex] = material;
                    }
                    else
                    {
                        material = matFound->second;
                        materialName = material.get()->GetMaterialName();
                    }

                    Events::ProcessingResult materialResult;
                    Containers::SceneGraph::NodeIndex newIndex =
                        context.m_scene.GetGraph().AddChild(context.m_currentGraphPosition, materialName.c_str());

                    AZ_Assert(newIndex.IsValid(), "Failed to create SceneGraph node for attribute.");
                    if (!newIndex.IsValid())
                    {
                        combinedMaterialImportResults += Events::ProcessingResult::Failure;
                        continue;
                    }

                    AssImpSceneAttributeDataPopulatedContext dataPopulated(context, material, newIndex, materialName);
                    materialResult = Events::Process(dataPopulated);

                    if (materialResult != Events::ProcessingResult::Failure)
                    {
                        materialResult = SceneAPI::SceneBuilder::AddAttributeDataNodeWithContexts(dataPopulated);
                    }

                    combinedMaterialImportResults += materialResult;
                }

                return combinedMaterialImportResults.GetResult();
            }

            AZStd::string AssImpMaterialImporter::ResolveTexturePath(const AZStd::string& sceneFilePath, const AZStd::string& textureFilePath) const
            {
                if (textureFilePath.empty())
                {
                    return textureFilePath;
                }


                AZStd::string texturePathRelativeToScene;
                AZStd::string cleanedUpSceneFilePath(sceneFilePath);
                AZ::StringFunc::Path::StripFullName(cleanedUpSceneFilePath);
                AZ::StringFunc::Path::Join(cleanedUpSceneFilePath.c_str(), textureFilePath.c_str(), texturePathRelativeToScene);

                // If the texture did start with marker to change directories upward, then it's relative to the scene file,
                // and that path needs to be resolved. It can't be resolved later. This was handled internally by FBX SDK,
                // but is not handled by AssImp.
                if (textureFilePath.starts_with(".."))
                {
                    // Not checking for the file existing because it may not be there yet.
                    return texturePathRelativeToScene;
                }

                bool ok;
                AZStd::string relativePath, rootPath;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                    ok, &AzToolsFramework::AssetSystemRequestBus::Events::GenerateRelativeSourcePath,
                    texturePathRelativeToScene.c_str(), relativePath, rootPath);

                // The engine only supports relative paths to scan folders.
                // Scene files may have paths to textures, relative to the scene file.
                // Try to use a scanfolder relative path instead
                if (ok)
                {
                    return relativePath;
                }


                return textureFilePath;
            }
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
