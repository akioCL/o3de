/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>
#include <Atom/RHI/DrawListTagRegistry.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/Pass/PassAttachment.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/Pass/RasterPassData.h>
#include <AzCore/std/iterator.h>

#include "LyShinePass.h"

namespace LyShine
{
    namespace
    {
        AZ::Name GetRenderTargetName(AZ::Data::Instance<AZ::RPI::AttachmentImage> attachmentImage)
        {
            AZ::Name renderTargetName = attachmentImage->GetRHIImage()->GetName();
            return renderTargetName;
        }
    }

    AZ::RPI::Ptr<LyShinePass> LyShinePass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        return aznew LyShinePass(descriptor);
    }

    LyShinePass::LyShinePass(const AZ::RPI::PassDescriptor& descriptor)
        : Base(descriptor)
    {
    }

    LyShinePass::~LyShinePass()
    {
        LyShinePassRequestBus::Handler::BusDisconnect();
    }

    void LyShinePass::ResetRenderTargets()
    {
        m_renderTargets.clear();
    }

    void LyShinePass::AddRenderTarget(AZ::Name name, AZ::Data::Instance<AZ::RPI::AttachmentImage> attachmentImage)
    {
        m_renderTargets[name] = attachmentImage;
        m_needsRebuild = true;
    }

    void LyShinePass::ResetInternal()
    {
        LyShinePassRequestBus::Handler::BusDisconnect();
    }

    void LyShinePass::BuildInternal()
    {
        RemoveChildren();

        // Get the current list of render targets being used across all UI Canvases
        LyShine::AttachmentImagesAndDependents attachmentImages;
        LyShinePassDataRequestBus::BroadcastResult( 
            attachmentImages,
            &LyShinePassDataRequestBus::Events::GetRenderTargets);

        AddRttChildPasses(attachmentImages);
        AddUiCanvasChildPass(attachmentImages);

        Base::BuildInternal();

        AZ::RPI::Scene* scene = GetScene();
        AZ_Assert(scene, "Attempting to get LyShinePass scene before it was initialized.");
        if (scene)
        {
            LyShinePassRequestBus::Handler::BusConnect(scene->GetId());
        }
    }

    void LyShinePass::ImageAttachmentsChanged()
    {
        QueueForBuildAndInitialization();
    }

    void LyShinePass::AddRttChildPasses(LyShine::AttachmentImagesAndDependents attachmentImagesAndDependents)
    {
        for (const auto& attachmentImageAndDescendents : attachmentImagesAndDependents)
        {
            AddRttChildPass(attachmentImageAndDescendents.first, attachmentImageAndDescendents.second);
        }
    }

    void LyShinePass::AddRttChildPass(AZ::Data::Instance<AZ::RPI::AttachmentImage> attachmentImage, AttachmentImages dependentAttachmentImages)
    {
        // Add a pass that renders to the specified texture

        // Create a pass template
        auto passTemplate = AZStd::make_shared<AZ::RPI::PassTemplate>();
        passTemplate->m_name = "RttChildPass";
        passTemplate->m_passClass = AZ::Name("RttChildPass");

        // Slots
        passTemplate->m_slots.resize(2);

        AZ::RPI::PassSlot& depthInOutSlot = passTemplate->m_slots[0];
        depthInOutSlot.m_name = "DepthInputOutput";
        depthInOutSlot.m_slotType = AZ::RPI::PassSlotType::InputOutput;
        depthInOutSlot.m_scopeAttachmentUsage = AZ::RHI::ScopeAttachmentUsage::DepthStencil;
        depthInOutSlot.m_loadStoreAction.m_clearValue = AZ::RHI::ClearValue::CreateDepthStencil(0.0f, 0);
        depthInOutSlot.m_loadStoreAction.m_loadActionStencil = AZ::RHI::AttachmentLoadAction::Clear;

        AZ::RPI::PassSlot& outSlot = passTemplate->m_slots[1];
        outSlot.m_name = AZ::Name("RenderTargetOutput");
        outSlot.m_slotType = AZ::RPI::PassSlotType::Output;
        outSlot.m_scopeAttachmentUsage = AZ::RHI::ScopeAttachmentUsage::RenderTarget;

        // Connections
        passTemplate->m_connections.resize(1);

        AZ::RPI::PassConnection& depthInOutConnection = passTemplate->m_connections[0];
        depthInOutConnection.m_localSlot = "DepthInputOutput";
        depthInOutConnection.m_attachmentRef.m_pass = "Parent";
        depthInOutConnection.m_attachmentRef.m_attachment = "DepthInputOutput";

        // Pass data
        AZStd::shared_ptr<AZ::RPI::RasterPassData> passData = AZStd::make_shared<AZ::RPI::RasterPassData>();
        passData->m_drawListTag = GetRenderTargetName(attachmentImage);
        passData->m_pipelineViewTag = AZ::Name("MainCamera");
        passTemplate->m_passData = AZStd::move(passData);

        // Create a pass descriptor for the new child pass
        AZ::RPI::PassDescriptor childDesc;
        childDesc.m_passTemplate = passTemplate;
        childDesc.m_passName = AZ::Name("RttChildPass");

        AZ::RPI::PassSystemInterface* passSystem = AZ::RPI::PassSystemInterface::Get();
        AZ::RPI::Ptr<AZ::RPI::Pass> rttChildPass = passSystem->CreatePass<RttChildPass>(childDesc);
        AZ_Assert(rttChildPass, "[LyShinePass] Unable to create %s.", passTemplate->m_name.GetCStr());

        (static_cast<RttChildPass*>(rttChildPass.get()))->m_attachmentImage = attachmentImage;
        (static_cast<RttChildPass*>(rttChildPass.get()))->m_dependentAttachmentImages = dependentAttachmentImages;

        AddChild(rttChildPass);
    }

    void LyShinePass::AddUiCanvasChildPass(LyShine::AttachmentImagesAndDependents attachmentImagesAndDependents)
    {
        if (!m_uiCanvasChildPass)
        {
            // Create a pass template
            auto passTemplate = AZStd::make_shared<AZ::RPI::PassTemplate>();
            passTemplate->m_name = AZ::Name("UiCanvasChildPass");
            passTemplate->m_passClass = AZ::Name("UiCanvasChildPass");

            // Slots
            passTemplate->m_slots.resize(2);

            AZ::RPI::PassSlot& depthInOutSlot = passTemplate->m_slots[0];
            depthInOutSlot.m_name = "DepthInputOutput";
            depthInOutSlot.m_slotType = AZ::RPI::PassSlotType::InputOutput;
            depthInOutSlot.m_scopeAttachmentUsage = AZ::RHI::ScopeAttachmentUsage::DepthStencil;
            depthInOutSlot.m_loadStoreAction.m_clearValue = AZ::RHI::ClearValue::CreateDepthStencil(0.0f, 0);
            depthInOutSlot.m_loadStoreAction.m_loadActionStencil = AZ::RHI::AttachmentLoadAction::Clear;

            AZ::RPI::PassSlot& inOutSlot = passTemplate->m_slots[1];
            inOutSlot.m_name = "InputOutput";
            inOutSlot.m_slotType = AZ::RPI::PassSlotType::InputOutput;
            inOutSlot.m_scopeAttachmentUsage = AZ::RHI::ScopeAttachmentUsage::RenderTarget;

            // Connections
            passTemplate->m_connections.resize(2);

            AZ::RPI::PassConnection& depthInOutConnection = passTemplate->m_connections[0];
            depthInOutConnection.m_localSlot = "DepthInputOutput";
            depthInOutConnection.m_attachmentRef.m_pass = "Parent";
            depthInOutConnection.m_attachmentRef.m_attachment = "DepthInputOutput";

            AZ::RPI::PassConnection& inOutConnection = passTemplate->m_connections[1];
            inOutConnection.m_localSlot = "InputOutput";
            inOutConnection.m_attachmentRef.m_pass = "Parent";
            inOutConnection.m_attachmentRef.m_attachment = "InputOutput";

            // Pass data
            AZStd::shared_ptr<AZ::RPI::RasterPassData> passData = AZStd::make_shared<AZ::RPI::RasterPassData>();
            passData->m_drawListTag = AZ::Name("uicanvas");
            passData->m_pipelineViewTag = AZ::Name("MainCamera");
            passTemplate->m_passData = AZStd::move(passData);

            // Create a pass descriptor for the new child pass
            AZ::RPI::PassDescriptor childDesc;
            childDesc.m_passTemplate = passTemplate;
            childDesc.m_passName = AZ::Name("UiCanvasChildPass");

            AZ::RPI::PassSystemInterface* passSystem = AZ::RPI::PassSystemInterface::Get();
            m_uiCanvasChildPass = passSystem->CreatePass<UiCanvasChildPass>(childDesc);
            AZ_Assert(m_uiCanvasChildPass, "[LyShinePass] Unable to create %s.", passTemplate->m_name.GetCStr());
        }

        m_uiCanvasChildPass->m_dependentAttachmentImages.clear();
        for (const auto& attachmentImageAndDescendents : attachmentImagesAndDependents)
        {
            m_uiCanvasChildPass->m_dependentAttachmentImages.emplace_back(attachmentImageAndDescendents.first);
        }

        AddChild(m_uiCanvasChildPass);
    }

    void LyShinePass::AddAttachmentImage(const AZ::Name& attachmentImageName, uint32_t width, uint32_t height)
    {
        AZ::RHI::ImageDescriptor imageDesc;
        imageDesc.m_bindFlags = AZ::RHI::ImageBindFlags::ShaderReadWrite;
        imageDesc.m_size.m_width = width;
        imageDesc.m_size.m_height = height;
        imageDesc.m_format = AZ::RHI::Format::R8G8B8A8_UNORM;

        AZ::Data::Instance<AZ::RPI::AttachmentImagePool> pool = AZ::RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();
        AZ::Data::Instance<AZ::RPI::AttachmentImage> attachmentImage = AZ::RPI::AttachmentImage::Create(*pool.get(), imageDesc,
            attachmentImageName);

        m_attachmentImages[attachmentImageName] = attachmentImage;
    }

    void LyShinePass::RemoveAttachmentImage(const AZ::Name& attachmentName)
    {
        m_attachmentImages.erase(attachmentName);
    }

    LyShineChildBasePass::LyShineChildBasePass(const AZ::RPI::PassDescriptor& descriptor)
        : RasterPass(descriptor)
    {
        const AZ::RPI::RasterPassData* passData = AZ::RPI::PassUtils::GetPassData<AZ::RPI::RasterPassData>(descriptor);
        if (passData)
        {
            auto* rhiSystem = AZ::RHI::RHISystemInterface::Get();
            m_drawListTag = rhiSystem->GetDrawListTagRegistry()->AcquireTag(passData->m_drawListTag);
        }
    }

    LyShineChildBasePass::~LyShineChildBasePass()
    {
        if (m_drawListTag.IsValid())
        {
            auto* rhiSystem = AZ::RHI::RHISystemInterface::Get();
            rhiSystem->GetDrawListTagRegistry()->ReleaseTag(m_drawListTag);
        }
    }

    void LyShineChildBasePass::SetupFrameGraphDependencies(AZ::RHI::FrameGraphInterface frameGraph)
    {
        for (auto attachmentImage : m_dependentAttachmentImages)
        {
            frameGraph.UseAttachment(AZ::RHI::ImageScopeAttachmentDescriptor{ attachmentImage->GetAttachmentId() },
                AZ::RHI::ScopeAttachmentAccess::Read,
                AZ::RHI::ScopeAttachmentUsage::Shader);
        }

        AZ::RPI::RasterPass::SetupFrameGraphDependencies(frameGraph);
    }

    AZ::RPI::Ptr<RttChildPass> RttChildPass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        return aznew RttChildPass(descriptor);
    }

    RttChildPass::RttChildPass(const AZ::RPI::PassDescriptor& descriptor)
        : LyShineChildBasePass(descriptor)
    {
    }

    void RttChildPass::BuildInternal()
    {
        AttachImageToSlot(AZ::Name("RenderTargetOutput"), m_attachmentImage);
    }

    RttChildPass::~RttChildPass()
    {
    }

    AZ::RPI::Ptr<UiCanvasChildPass> UiCanvasChildPass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        return aznew UiCanvasChildPass(descriptor);
    }

    UiCanvasChildPass::UiCanvasChildPass(const AZ::RPI::PassDescriptor& descriptor)
        : LyShineChildBasePass(descriptor)
    {
    }

    UiCanvasChildPass::~UiCanvasChildPass()
    {
    }
} // namespace LyShine
