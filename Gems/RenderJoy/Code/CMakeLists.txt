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

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_target(
        NAME RenderJoy.Editor.Static STATIC
        NAMESPACE Gem
        AUTOMOC
        AUTOUIC
        FILES_CMAKE
            renderjoy_editor_static_files.cmake
        PLATFORM_INCLUDE_FILES
            Source/Platform/Common/${PAL_TRAIT_COMPILER_ID}/editor_${PAL_TRAIT_COMPILER_ID_LOWERCASE}.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                RENDERJOY_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                .
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                3rdParty::Qt::Core
                3rdParty::Qt::Gui
                3rdParty::Qt::Widgets
                AZ::AzCore
                AZ::AzToolsFramework
                Legacy::Editor.Headers
                Legacy::EditorLib
                Gem::AtomToolsFramework.Static
                Gem::AtomToolsFramework.Editor
                Gem::Atom_RHI.Public
                Gem::Atom_RPI.Public
                Gem::Atom_Feature_Common.Static
                Gem::Atom_Component_DebugCamera.Static
                Gem::AtomLyIntegration_CommonFeatures.Static
    )

    ly_add_target(
        NAME RenderJoy.Editor MODULE
        NAMESPACE Gem
        OUTPUT_NAME Gem.RenderJoy.Editor.deda12623060436ca264dec26835db06.v0.1.0
        FILES_CMAKE
            renderjoy_editor_files.cmake
        COMPILE_DEFINITIONS
            PRIVATE
                RENDERJOY_EDITOR
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PRIVATE
                AZ::AzCore
                AZ::AzToolsFramework
                Legacy::Editor.Headers
                Gem::AtomToolsFramework.Static
                Gem::AtomToolsFramework.Editor
                Gem::Atom_RHI.Public
                Gem::Atom_RPI.Public
                Gem::Atom_Feature_Common.Static
                Gem::Atom_Component_DebugCamera.Static
                Gem::AtomLyIntegration_CommonFeatures.Static
                Gem::RenderJoy.Editor.Static
    )
endif()

################################################################################
# Tests
################################################################################
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        ly_add_target(
            NAME RenderJoy.Editor.Tests MODULE
            NAMESPACE Gem
            FILES_CMAKE
                renderjoy_tests_editor_files.cmake
            COMPILE_DEFINITIONS
                PRIVATE
                    RENDERJOY_EDITOR
            INCLUDE_DIRECTORIES
                PRIVATE
                    .
                    Tests
                    Source
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    AZ::AzToolsFramework
                    Gem::RenderJoy.Editor.Static
        )
        ly_add_googletest(
            NAME Gem::RenderJoy.Editor.Tests
        )
    endif()
endif()
