/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentId.h>
#include <Atom/RHI.Reflect/ClearValue.h>
#include <RHI/RenderPass.h>
#include <RHI/Framebuffer.h>
#include <RHI/Scope.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/array.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;
        class Scope;

        struct RenderPassContext
        {
            RHI::Ptr<Framebuffer> m_framebuffer;
            RHI::Ptr<RenderPass> m_renderPass;
            AZStd::vector<RHI::ClearValue> m_clearValues;

            bool IsValid() const;
            void SetName(const AZ::Name& name);
        };

        //! Utility class that builds a renderpass and a framebuffer from the
        //! resource attachments of a Scope. It uses the load and store actions and the clear colors
        //! to build the proper renderpass that will be used for rendering.
        class RenderPassBuilder
        {
        public:
            RenderPassBuilder(Device& device, uint32_t subpassesCount);
            ~RenderPassBuilder() = default;

            //! Adds the attachments that are used by the Scope into the renderpass descriptor.
            void AddScopeAttachments(Scope& scope);

            //! Builds the renderpass and framebuffer from the information collected from the scopes.
            RHI::ResultCode End(RenderPassContext& builtContext);

            //! Returns if the builder has the attachments to build the framebuffer and renderpass.
            bool CanBuild() const;

        private:
            VkImageLayout GetInitialLayout(const Scope& scope, const RHI::ImageScopeAttachment& attachment) const;
            VkImageLayout GetFinalLayout(const Scope& scope, const RHI::ImageScopeAttachment& attachment) const;

            Device& m_device;
            AZStd::unordered_map<RHI::AttachmentId, uint32_t> m_attachmentsMap;
            AZStd::array<RenderPass::AttachmentUsageInfo, RHI::Limits::Pipeline::RenderAttachmentCountMax> m_lastSubpassResourceUse;
            RenderPass::Descriptor m_renderpassDesc;
            Framebuffer::Descriptor m_framebufferDesc;
            AZStd::vector<RHI::ClearValue> m_clearValues;
            AZStd::vector<AZStd::array<RenderPass::AttachmentUsageInfo, RHI::Limits::Pipeline::RenderAttachmentCountMax>>
                m_usedAttachmentsPerSubpass;
            uint32_t m_subpassCount = 0;
        };
    }
}
