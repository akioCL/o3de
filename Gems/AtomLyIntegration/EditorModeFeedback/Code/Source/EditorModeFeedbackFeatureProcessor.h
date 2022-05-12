/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>

#include <Pass/EditorModeTintPass.h>

namespace AZ
{
    namespace Render
    {
        //! Feature processor for Editor Mode Feedback visual effect system.
        class EditorModeFeatureProcessor
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::EditorModeFeatureProcessor, "{78D40D57-F564-4ECD-B9F5-D8C9784B15D0}", RPI::FeatureProcessor);

            static void Reflect(AZ::ReflectContext* context);

            EditorModeFeatureProcessor();

            // FeatureProcessor overrides ...
            void Activate() override;
            void Deactivate() override;
            void ApplyRenderPipelineChange(RPI::RenderPipeline* renderPipeline) override;
            void Simulate(const SimulatePacket& packet) override;

            // RPI::SceneNotificationBus overrides ...
            void OnRenderPipelineAdded(RPI::RenderPipelinePtr pipeline) override;
            void OnRenderPipelinePassesChanged(RPI::RenderPipeline* renderPipeline) override;

        private:
            void InitPasses(RPI::RenderPipeline* renderPipeline);
            EditorModeTintPass* m_tintPass1 = nullptr;
            EditorModeTintPass* m_tintPass2 = nullptr;

            class PassSystem
            {
            public:
                PassSystem(const Name& passName, const Name& passAttachment);

                void AddStatePass(
                    const AZStd::string& stateName, const AZStd::string& passParentTemplateName, RPI::RenderPipeline* renderPipeline);

            private:
                Name m_lastStatePass;
                Name m_lastStateAttachment;
            };

            PassSystem m_passSystem;

            //! Cache the pass request data for creating an editor mode feedback parent pass.
            AZ::Data::Asset<AZ::RPI::AnyAsset> m_parentPassRequestAsset;
        };
    } // namespace Render
} // namespace AZ
