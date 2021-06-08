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

#include "BaseManipulator.h"

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    class ManipulatorView;

    class BrushManipulator : public BaseManipulator, public ManipulatorSpaceWithLocalTransform
    {
        //! Private constructor.
        BrushManipulator(const AZ::Transform& worldFromLocal);

    public:
        AZ_RTTI(BrushManipulator, "{0621CB58-21FD-474A-A296-5B1192E714E7}", BaseManipulator);
        AZ_CLASS_ALLOCATOR(BrushManipulator, AZ::SystemAllocator, 0);

        BrushManipulator() = delete;
        BrushManipulator(const BrushManipulator&) = delete;
        BrushManipulator& operator=(const BrushManipulator&) = delete;

        ~BrushManipulator() = default;

        //! A Manipulator must only be created and managed through a shared_ptr.
        static AZStd::shared_ptr<BrushManipulator> MakeShared(const AZ::Transform& worldFromLocal);

        //! The state of the manipulator at the start of an interaction.
        struct Start
        {
        };

        //! The state of the manipulator during an interaction.
        struct Current
        {
        };

        //! Mouse action data used by MouseActionCallback (wraps Start and Current manipulator state).
        struct Action
        {
            Start m_start;
            Current m_current;
            ViewportInteraction::KeyboardModifiers m_modifiers;
        };

        void Draw(
            const ManipulatorManagerState& managerState, AzFramework::DebugDisplayRequests& debugDisplay,
            const AzFramework::CameraState& cameraState, const ViewportInteraction::MouseInteraction& mouseInteraction) override;

        void SetView(AZStd::shared_ptr<ManipulatorView> view);

    private:
        struct StartInternal
        {
            AZ::Transform m_worldFromLocal;
        };

        struct CurrentInternal
        {
        };

        struct ActionInternal
        {
            StartInternal m_start;
            CurrentInternal m_current;
        };

        ActionInternal m_actionInternal;

        AZStd::shared_ptr<ManipulatorView> m_manipulatorView;
    };
} // namespace AzToolsFramework
