/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzFramework/Entity/SliceEntityOwnershipService.h>
#include <AzFramework/Entity/SliceGameEntityOwnershipServiceBus.h>

namespace AzFramework
{
    class SliceGameEntityOwnershipService
        : public SliceEntityOwnershipService
        , private SliceGameEntityOwnershipServiceRequestBus::Handler
        , private SliceInstantiationResultBus::MultiHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(SliceGameEntityOwnershipService, AZ::SystemAllocator, 0);
        explicit SliceGameEntityOwnershipService(const EntityContextId& entityContextId, AZ::SerializeContext* serializeContext);

        virtual ~SliceGameEntityOwnershipService();

        static void Reflect(AZ::ReflectContext* context);

        void Reset() override;

        //////////////////////////////////////////////////////////////////////////
        // SliceGameEntityOwnershipServiceRequestBus::Handler
        SliceInstantiationTicket InstantiateDynamicSlice(const AZ::Data::Asset<AZ::Data::AssetData>& sliceAsset,
            const AZ::Transform& worldTransform, const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customIdMapper) override;
        AZ::SliceComponent::SliceInstanceAddress InstantiateDynamicSliceBlocking(const AZ::Data::Asset<AZ::Data::AssetData>& /*sliceAsset*/,
            const AZ::Transform& /*worldTransform*/, const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& /*customIdMapper*/,
            const AZStd::function<void(const AZ::SliceComponent::SliceInstanceAddress&)>& /*preInstantiate*/) override;
        void CancelDynamicSliceInstantiation(const SliceInstantiationTicket& ticket) override;
        bool DestroyDynamicSliceByEntity(const AZ::EntityId&) override;
        //////////////////////////////////////////////////////////////////////////
        
        //////////////////////////////////////////////////////////////////////////
        // SliceInstantiationResultBus::MultiHandler
        void OnSlicePreInstantiate(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& instance) override;
        void OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, const AZ::SliceComponent::SliceInstanceAddress& instance) override;
        void OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId) override;
        //////////////////////////////////////////////////////////////////////////
    protected:
        void CreateRootSlice() override;

    private:
        void FlushDynamicSliceDeletionList();

        /**
         * Specifies that a given entity should not be activated by default
         * after it is created.
         * @param entityId The entity that should not be activated by default.
         */
        void MarkEntityForNoActivation(AZ::EntityId entityId);

        struct InstantiatingDynamicSliceInfo
        {
            AZ::Data::Asset<AZ::Data::AssetData> m_asset;
            AZ::Transform m_transform;
        };

        void OnSlicePreInstantiate(const InstantiatingDynamicSliceInfo& sliceInfo, const AZ::SliceComponent::SliceInstanceAddress& instance);

        AZStd::unordered_map<SliceInstantiationTicket, InstantiatingDynamicSliceInfo> m_instantiatingDynamicSlices;

        SliceInstanceUnorderedSet m_dynamicSlicesToDestroy;
    };
}
