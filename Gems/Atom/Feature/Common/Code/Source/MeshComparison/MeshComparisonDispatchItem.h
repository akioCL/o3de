/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RHI/DispatchItem.h>
#include <AtomCore/Instance/Instance.h>

namespace AZ
{
    namespace Render
    {
        //! A class for creating the dispatch item(s) necessary to compare two meshes
        class MeshComparisonDispatchItem
        {
        public:
            AZ_RTTI(AZ::Render::MeshComparisonDispatchItem, "{EB338178-722B-4071-BD1B-FD08C473ECB8}");

            MeshComparisonDispatchItem() = delete;
            MeshComparisonDispatchItem(
                const Data::Asset<RPI::ModelAsset>& modelAssetA,
                const Data::Asset<RPI::ModelAsset>& modelAssetB,
                const Data::Asset<RPI::ModelAsset>& comparisonModelAsset);
            virtual ~MeshComparisonDispatchItem();

            bool Init();

        private:
            Data::Asset<RPI::ModelAsset> m_modelAssetA;
            Data::Asset<RPI::ModelAsset> m_modelAssetB;
            Data::Asset<RPI::ModelAsset> m_comparisonModelAsset;
            Data::Instance<RPI::Model> m_modelA;
            Data::Instance<RPI::Model> m_modelB;
            Data::Instance<RPI::Model> m_comparisonModel;

            Data::Instance<RPI::Shader> m_triangleDistanceShader;

            AZStd::vector<AZ::RHI::Ptr<AZ::RHI::BufferView>> m_positionBufferViews;
            AZStd::vector<AZ::RHI::Ptr<AZ::RHI::BufferView>> m_indexBufferViews;
            AZStd::vector<RHI::DispatchItem> m_triangleDistanceDispatchItems;
            AZStd::vector<Data::Instance<RPI::ShaderResourceGroup>> m_triangleDistanceInstanceSrgs;
        };
    } // namespace Render
} // namespace AZ
