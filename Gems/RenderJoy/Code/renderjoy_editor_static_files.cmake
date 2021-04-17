#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(FILES
    Include/RenderJoy/RenderJoyCommon.h
    Include/RenderJoy/RenderJoySettingsBus.h
    Include/RenderJoy/RenderJoyTextureProviderBus.h
    Include/RenderJoy/RenderJoyPassBus.h
    Include/RenderJoy/IRenderJoySrgDataProvider.h
    Include/RenderJoy/RenderJoyKeyboardBus.h
    Source/RenderJoySystemComponent.cpp
    Source/RenderJoySystemComponent.h
    Source/MainWindow.cpp
    Source/MainWindow.h
    Source/RenderJoyTrianglePass.cpp
    Source/RenderJoyTrianglePass.h
    Source/RenderJoyMainPassTemplateCreator.cpp
    Source/RenderJoyMainPassTemplateCreator.h
    Source/Viewport/RenderJoyViewportRenderer.cpp
    Source/Viewport/RenderJoyViewportRenderer.h
    Source/Viewport/RenderJoyViewportWidget.cpp
    Source/Viewport/RenderJoyViewportWidget.h
    Source/Viewport/RenderJoyViewportWidget.ui
    Source/Components/RenderJoySettingsEditorComponent.cpp
    Source/Components/RenderJoySettingsEditorComponent.h
    Source/Components/RenderJoyPassEditorComponent.cpp
    Source/Components/RenderJoyPassEditorComponent.h
    Source/Components/RenderJoyTextureEditorComponent.cpp
    Source/Components/RenderJoyTextureEditorComponent.h
    Source/Components/RenderJoyKeyboardEditorComponent.cpp
    Source/Components/RenderJoyKeyboardEditorComponent.h
)
