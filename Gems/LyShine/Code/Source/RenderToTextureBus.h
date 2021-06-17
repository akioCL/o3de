/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace LyShine
{
    //! Ebus to handle render target requests
    class RenderToTextureRequests
        : public AZ::ComponentBus
    {
    public:
        virtual AZ::RHI::AttachmentId CreateRenderTarget(const AZ::Name& renderTargetName, AZ::RHI::Size size) = 0;
        virtual void DestroyRenderTarget(const AZ::RHI::AttachmentId& attachmentId) = 0;
        virtual AZ::Data::Instance<AZ::RPI::AttachmentImage> GetRenderTarget(const AZ::RHI::AttachmentId& attachmentId) = 0;
        virtual void ResizeRenderTarget(const AZ::RHI::AttachmentId& attachmentId, AZ::RHI::Size size) = 0;
    };

    using RenderToTextureRequestBus = AZ::EBus<RenderToTextureRequests>;
}
