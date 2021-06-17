/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/RasterPass.h>
#include <AtomCore/std/containers/array_view.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace RPI
    {
        class AttachmentImage;
    }
}

namespace LyShine
{
    using AttachmentImages = AZStd::vector<AZ::Data::Instance<AZ::RPI::AttachmentImage>>;
    using AttachmentImageAndDependentsPair = AZStd::pair<AZ::Data::Instance<AZ::RPI::AttachmentImage>, AttachmentImages>;
    using AttachmentImagesAndDependents = AZStd::vector<AttachmentImageAndDependentsPair>;
}

class LyShinePassRequests
    : public AZ::EBusTraits
{
public:
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
    using BusIdType = AZ::RPI::SceneId;

    //! Pass needs a rebuild
    virtual void ImageAttachmentsChanged() = 0;
};
using LyShinePassRequestBus = AZ::EBus<LyShinePassRequests>;

class LyShinePassDataRequests
    : public AZ::EBusTraits
{
public:
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
    using BusIdType = AZ::RPI::SceneId;

    //! Get a list of render targets for this pass
    virtual LyShine::AttachmentImagesAndDependents GetRenderTargets() = 0;
};
using LyShinePassDataRequestBus = AZ::EBus<LyShinePassDataRequests>;
