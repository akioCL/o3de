/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorModeFeedbackFeatureProcessor.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

namespace AZ
{
    namespace Render
    {
        namespace
        {
            const char* const Window = "EditorModeFeedback";
        }

        void EditorModeFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<EditorModeFeatureProcessor, RPI::FeatureProcessor>()->Version(0);
            }
        }

        void EditorModeFeatureProcessor::Activate()
        {
            EnableSceneNotification();
        }

        void EditorModeFeatureProcessor::Deactivate()
        {
            DisableSceneNotification();
            m_parentPassRequestAsset.Reset();
        }

        EditorModeFeatureProcessor::EditorModeFeatureProcessor()
            : m_passSystem(Name("PostProcessPass"), Name("Output"))
        {
        }

        EditorModeFeatureProcessor::PassSystem::PassSystem(const Name& passName, const Name& passAttachment)
            : m_lastStatePass(passName)
            , m_lastStateAttachment(passAttachment)
        {
        }

        void EditorModeFeatureProcessor::PassSystem::AddStatePass(const AZStd::string& stateName, const AZStd::string& passParentTemplateName, RPI::RenderPipeline* renderPipeline)
        {
            const Name parentTemplateName = Name(stateName + passParentTemplateName);
            const Name stateTemplateName = Name(passParentTemplateName);
            const Name passName = Name(stateName + "Parent");
            auto parentTemplate = RPI::PassSystemInterface::Get()->GetPassTemplate(Name("EditorModeFeedbackParentTemplate"));
            auto parentInstanceTemplate = parentTemplate->Clone();
            parentInstanceTemplate->m_name = parentTemplateName;
            //parentInstanceTemplate->m_passRequests[1].m_passName = Name(stateName); // used to find, coudl just find the parent then "EditorModeEffectPass"
            parentInstanceTemplate->m_passRequests[1].m_templateName = stateTemplateName;
            RPI::PassSystemInterface::Get()->AddPassTemplate(parentTemplateName, parentInstanceTemplate);

            AZ::RPI::PassRequest passRequest;
            passRequest.m_passName = passName;
            passRequest.m_templateName = parentTemplateName;
            passRequest.AddInputConnection(
                RPI::PassConnection{ Name("ColorInputOutput"), RPI::PassAttachmentRef{ m_lastStatePass, m_lastStateAttachment } });
            passRequest.AddInputConnection(
                RPI::PassConnection{ Name("InputDepth"), RPI::PassAttachmentRef{ Name("DepthPrePass"), Name("Depth") } });

            RPI::Ptr<RPI::Pass> parentPass = RPI::PassSystemInterface::Get()->CreatePassFromRequest(&passRequest);
            if (!parentPass)
            {
                AZ_Error(
                    Window, false, "Create editor mode feedback parent pass from pass request failed", renderPipeline->GetId().GetCStr());
                return;
            }

            // Inject the parent pass after the PostProcess pass
            if (!renderPipeline->AddPassAfter(parentPass, m_lastStatePass))
            {
                AZ_Error(
                    Window, false, "Add editor mode feedback parent pass to render pipeline [%s] failed",
                    renderPipeline->GetId().GetCStr());
                return;
            }

            m_lastStatePass = passName;
            m_lastStateAttachment = Name("ColorInputOutput");
        }

        void EditorModeFeatureProcessor::OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline)
        {
            InitPasses(pipeline.get());
        }

        void EditorModeFeatureProcessor::OnRenderPipelinePassesChanged([[maybe_unused]] RPI::RenderPipeline* renderPipeline)
        {
            InitPasses(renderPipeline);
        }

        void EditorModeFeatureProcessor::InitPasses([[maybe_unused]] RPI::RenderPipeline* renderPipeline)
        {
            {
                RPI::PassFilter focusPassFilter = RPI::PassFilter::CreateWithPassName(AZ::Name{ "FocusModeParent" }, renderPipeline);
                RPI::Ptr<RPI::Pass> focusPass = RPI::PassSystemInterface::Get()->FindFirstPass(focusPassFilter);
                if (!focusPass.get())
                    return;
                auto ppp = azdynamic_cast<RPI::ParentPass*>(focusPass.get());
                auto qqq = azdynamic_cast<RPI::ParentPass*> (ppp->FindChildPass(Name("EditorModeEffectPass")));
                RPI::Ptr<RPI::Pass> tintPass = qqq->FindChildPass(Name("TintPass"));
                m_tintPass1 = azdynamic_cast<EditorModeTintPass*>(tintPass.get());
            }
            {
                RPI::PassFilter focusPassFilter =
                    RPI::PassFilter::CreateWithPassName(AZ::Name{ "FakeModeParent" }, renderPipeline);
                RPI::Ptr<RPI::Pass> focusPass = RPI::PassSystemInterface::Get()->FindFirstPass(focusPassFilter);
                if (!focusPass.get())
                    return;
                auto ppp = azdynamic_cast<RPI::ParentPass*>(focusPass.get());
                auto qqq = azdynamic_cast<RPI::ParentPass*>(ppp->FindChildPass(Name("EditorModeEffectPass")));
                RPI::Ptr<RPI::Pass> tintPass = qqq->FindChildPass(Name("TintPass"));
                m_tintPass2 = azdynamic_cast<EditorModeTintPass*>(tintPass.get());
            }
        }

        void EditorModeFeatureProcessor::Simulate([[maybe_unused]] const SimulatePacket& packet)
        {
            if (m_tintPass1)
            {
                m_tintPass1->SetTintColor(AZ::Color(float(1.0), float(0.0), float(0.0), float(1.0)));
            }

            if (m_tintPass2)
            {
                m_tintPass2->SetTintColor(AZ::Color(float(0.0), float(0.0), float(1.0), float(1.0)));
            }
        }
       
        void EditorModeFeatureProcessor::ApplyRenderPipelineChange([[maybe_unused]]RPI::RenderPipeline* renderPipeline)
        {
            m_passSystem.AddStatePass("FocusMode", "EditorModeFocusParentTemplate", renderPipeline);
            m_passSystem.AddStatePass("FakeMode", "EditorModeFocusParentTemplate", renderPipeline);
        }
    } // namespace Render
} // namespace AZ
