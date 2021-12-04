/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <assimp/scene.h>
#include <assimp/material.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Sha1.h>
#include <AzCore/Debug/Trace.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SDKWrapper/AssImpMaterialWrapper.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace AssImpSDKWrapper
    {

        AssImpMaterialWrapper::AssImpMaterialWrapper(aiMaterial* aiMaterial)
            :m_assImpMaterial(aiMaterial)
        {
            AZ_Assert(aiMaterial, "Asset Importer Material cannot be null");
        }

        aiMaterial* AssImpMaterialWrapper::GetAssImpMaterial() const
        {
            return m_assImpMaterial;
        }

        AZStd::string AssImpMaterialWrapper::GetName() const
        {
            return m_assImpMaterial->GetName().C_Str();
        }

        AZ::u64 AssImpMaterialWrapper::GetUniqueId() const
        {
            AZStd::string fingerprintString;
            fingerprintString.append(GetName());
            AZStd::string extraInformation = AZStd::string::format("%i%i%i%i%i%i", m_assImpMaterial->GetTextureCount(aiTextureType_DIFFUSE),
                m_assImpMaterial->GetTextureCount(aiTextureType_SPECULAR), m_assImpMaterial->GetTextureCount(aiTextureType_NORMALS), m_assImpMaterial->GetTextureCount(aiTextureType_SHININESS),
                m_assImpMaterial->GetTextureCount(aiTextureType_AMBIENT), m_assImpMaterial->GetTextureCount(aiTextureType_EMISSIVE));
            fingerprintString.append(extraInformation);
            AZ::Sha1 sha;
            sha.ProcessBytes(fingerprintString.data(), fingerprintString.size());
            AZ::u32 digest[5]; //sha1 populate a 5 element array of AZ:u32
            sha.GetDigest(digest);
            return (static_cast<AZ::u64>(digest[0]) << 32) | digest[1];
        }

        AZ::Vector3 AssImpMaterialWrapper::GetDiffuseColor() const
        {
            aiColor3D color(1.f, 1.f, 1.f);
            if (m_assImpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color) == aiReturn::aiReturn_FAILURE)
            {
                AZ_Warning(AZ::SceneAPI::Utilities::WarningWindow, false, "Unable to get diffuse property from the material. Using default.");
            }
            return AZ::Vector3(color.r, color.g, color.b);
        }

        AZ::Vector3 AssImpMaterialWrapper::GetSpecularColor() const
        {
            aiColor3D color(0.f, 0.f, 0.f);
            if (m_assImpMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color) == aiReturn::aiReturn_FAILURE)
            {
                AZ_Warning(AZ::SceneAPI::Utilities::WarningWindow, false, "Unable to get specular property from the material. Using default.");
            }
            return AZ::Vector3(color.r, color.g, color.b);
        }

        AZ::Vector3 AssImpMaterialWrapper::GetEmissiveColor() const
        {
            aiColor3D color(0.f, 0.f, 0.f);
            if (m_assImpMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, color) == aiReturn::aiReturn_FAILURE)
            {
                AZ_Warning(AZ::SceneAPI::Utilities::WarningWindow, false, "Unable to get emissive property from the material. Using default.");
            }
            return AZ::Vector3(color.r, color.g, color.b);
        }

        float AssImpMaterialWrapper::GetOpacity() const
        {
            float opacity = 1.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_OPACITY, opacity) == aiReturn::aiReturn_FAILURE)
            {
                AZ_Warning(AZ::SceneAPI::Utilities::WarningWindow, false, "Unable to get opacity from the material. Using default.");
            }
            return opacity;
        }

        float AssImpMaterialWrapper::GetShininess() const
        {
            float shininess = 0.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_SHININESS, shininess) == aiReturn::aiReturn_FAILURE)
            {
                AZ_Warning(AZ::SceneAPI::Utilities::WarningWindow, false, "Unable to get shininess from the material. Using default.");
            }
            return shininess;
        }

        AZStd::optional<bool> AssImpMaterialWrapper::GetUseColorMap() const
        {
            // AssImp stores values as floats, so load as float and convert to bool.
            float useMap = 0.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_USE_COLOR_MAP, useMap) != aiReturn::aiReturn_FAILURE)
            {
                return useMap != 0.0f;
            }
            return AZStd::nullopt;
        }

        AZStd::optional<AZ::Vector3> AssImpMaterialWrapper::GetBaseColor() const
        {
            aiColor3D color(1.f, 1.f, 1.f);
            if (m_assImpMaterial->Get(AI_MATKEY_BASE_COLOR, color) != aiReturn::aiReturn_FAILURE)
            {
                return AZ::Vector3(color.r, color.g, color.b);
            }
            return AZStd::nullopt;
        }

        AZStd::optional<bool> AssImpMaterialWrapper::GetUseMetallicMap() const
        {
            // AssImp stores values as floats, so load as float and convert to bool.
            float useMap = 0.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_USE_METALLIC_MAP, useMap) != aiReturn::aiReturn_FAILURE)
            {
                return useMap != 0.0f;
            }
            return AZStd::nullopt;
        }

        AZStd::optional<float> AssImpMaterialWrapper::GetMetallicFactor() const
        {
            float metallic = 0.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metallic) != aiReturn::aiReturn_FAILURE)
            {
                return metallic;
            }
            return AZStd::nullopt;
        }

        AZStd::optional<bool> AssImpMaterialWrapper::GetUseRoughnessMap() const
        {
            // AssImp stores values as floats, so load as float and convert to bool.
            float useMap = 0.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_USE_ROUGHNESS_MAP, useMap) != aiReturn::aiReturn_FAILURE)
            {
                return useMap != 0.0f;
            }
            return AZStd::nullopt;
        }

        AZStd::optional<float> AssImpMaterialWrapper::GetRoughnessFactor() const
        {
            float roughness = 0.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) != aiReturn::aiReturn_FAILURE)
            {
                return roughness;
            }
            return AZStd::nullopt;
        }

        AZStd::optional<bool> AssImpMaterialWrapper::GetUseEmissiveMap() const
        {
            // AssImp stores values as floats, so load as float and convert to bool.
            float useMap = 0.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_USE_EMISSIVE_MAP, useMap) != aiReturn::aiReturn_FAILURE)
            {
                return useMap != 0.0f;
            }
            return AZStd::nullopt;
        }

        AZStd::optional<float> AssImpMaterialWrapper::GetEmissiveIntensity() const
        {
            float emissiveIntensity = 0.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveIntensity) != aiReturn::aiReturn_FAILURE)
            {
                return emissiveIntensity;
            }
            return AZStd::nullopt;
        }

        AZStd::optional<bool> AssImpMaterialWrapper::GetUseAOMap() const
        {
            // AssImp stores values as floats, so load as float and convert to bool.
            float useMap = 0.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_USE_AO_MAP, useMap) != aiReturn::aiReturn_FAILURE)
            {
                return useMap != 0.0f;
            }
            return AZStd::nullopt;
        }
        
        bool AssImpMaterialWrapper::GetTwoSided() const
        {
            bool twoSided = false;
            m_assImpMaterial->Get(AI_MATKEY_TWOSIDED, twoSided);
            return twoSided;
        }

        AZStd::string AssImpMaterialWrapper::GetTextureFileName(MaterialMapType textureType) const
        {
            aiString absTexturePath{""};

            auto getTextureFile = [this, &absTexturePath](aiTextureType textureType)
            {
                /// Engine currently doesn't support multiple textures. Right now we only use first texture.
                unsigned int textureIndex = 0;
                
                if (m_assImpMaterial->GetTextureCount(textureType) > textureIndex)
                {
                    m_assImpMaterial->GetTexture(textureType, textureIndex, &absTexturePath);
                }
            };

            switch (textureType)
            {
            case MaterialMapType::Diffuse:
                getTextureFile(aiTextureType_DIFFUSE);
                break;
            case MaterialMapType::Specular:
                getTextureFile(aiTextureType_SPECULAR);
                break;
            case MaterialMapType::Bump:
                getTextureFile(aiTextureType_HEIGHT);
                break;
            case MaterialMapType::Normal:
                getTextureFile(aiTextureType_NORMAL_CAMERA);
                getTextureFile(aiTextureType_NORMALS); // will override aiTextureType_NORMAL_CAMERA if present
                break;
            case MaterialMapType::Metallic:
                getTextureFile(aiTextureType_METALNESS);
                break;
            case MaterialMapType::Roughness:
                getTextureFile(aiTextureType_DIFFUSE_ROUGHNESS);
                break;
            case MaterialMapType::AmbientOcclusion:
                getTextureFile(aiTextureType_AMBIENT_OCCLUSION);
                break;
            case MaterialMapType::Emissive:
                getTextureFile(aiTextureType_EMISSION_COLOR);
                break;
            case MaterialMapType::BaseColor:
                getTextureFile(aiTextureType_DIFFUSE);    // this is the fallback, older property
                getTextureFile(aiTextureType_BASE_COLOR); // will override aiTextureType_DIFFUSE if present
                break;
            default:
                AZ_TraceContext("Unknown value", aznumeric_cast<int>(textureType));
                AZ_TracePrintf(SceneAPI::Utilities::WarningWindow, "Unrecognized MaterialMapType retrieved");
            }
            
            return AZStd::string(absTexturePath.C_Str());
        }
        
        AZ::u32 AssImpMaterialWrapper::GetTextureFlags(MaterialMapType textureType) const
        {
            AZ::u32 textureFlags = 0u;

            auto getTextureFlag = [this, &textureFlags](aiTextureType textureType)
            {
                /// Engine currently doesn't support multiple textures. Right now we only use first texture.
                unsigned int textureIndex = 0;

                if (m_assImpMaterial->GetTextureCount(textureType) > textureIndex)
                {
                    m_assImpMaterial->Get(AI_MATKEY_TEXFLAGS(textureType, textureIndex), textureFlags);
                }
            };

            switch (textureType)
            {
            case MaterialMapType::Diffuse:
                getTextureFlag(aiTextureType_DIFFUSE);
                break;
            case MaterialMapType::Specular:
                getTextureFlag(aiTextureType_SPECULAR);
                break;
            case MaterialMapType::Bump:
                getTextureFlag(aiTextureType_HEIGHT);
                break;
            case MaterialMapType::Normal:
                getTextureFlag(aiTextureType_NORMAL_CAMERA);
                getTextureFlag(aiTextureType_NORMALS); // will override aiTextureType_NORMAL_CAMERA if present
                break;
            case MaterialMapType::Metallic:
                getTextureFlag(aiTextureType_METALNESS);
                break;
            case MaterialMapType::Roughness:
                getTextureFlag(aiTextureType_DIFFUSE_ROUGHNESS);
                break;
            case MaterialMapType::AmbientOcclusion:
                getTextureFlag(aiTextureType_AMBIENT_OCCLUSION);
                break;
            case MaterialMapType::Emissive:
                getTextureFlag(aiTextureType_EMISSION_COLOR);
                break;
            case MaterialMapType::BaseColor:
                getTextureFlag(aiTextureType_DIFFUSE);    // this is the fallback, older property
                getTextureFlag(aiTextureType_BASE_COLOR); // will override aiTextureType_DIFFUSE if present
                break;
            default:
                AZ_TraceContext("Unknown value", aznumeric_cast<int>(textureType));
                AZ_TracePrintf(SceneAPI::Utilities::WarningWindow, "Unrecognized MaterialMapType retrieved");
            }

            return textureFlags;
        }

    } // namespace AssImpSDKWrapper
} // namespace AZ
