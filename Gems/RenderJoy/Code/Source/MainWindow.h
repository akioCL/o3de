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

#if !defined(Q_MOC_RUN)

// AZ
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/PlatformDef.h>
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzCore/Component/TickBus.h>

// Qt
#include <QtWidgets/QMainWindow>

#endif

namespace RenderJoy
{
    class RenderJoyViewportWidget;

    class MainWindow
        : public QMainWindow
        , public AZ::TickBus::Handler
    {
        Q_OBJECT

    public:
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow() override;

        // AZ::TickBus::Handler interface overrides...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        QSize sizeHint() const override
        {
            return QSize(800, 450);
        }


    private:

        RenderJoyViewportWidget* m_viewport = nullptr;
    };
} 
