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

#! ly_add_shader: adds a shader to be compiled
function(ly_add_shader)
    
    # Parse the arguments for the function calls ------------------------------------------------------
    set(options)
    set(oneValueArgs NAME AZSL ENTRY_VS ENTRY_PS)
    set(multiValueArgs)

    cmake_parse_arguments(ly_add_shader "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate input arguments ------------------------------------------------------------------------
    if(NOT ly_add_shader_NAME)
        message(FATAL_ERROR "You must provide a name for the target")
    endif()
    if(NOT ly_add_shader_AZSL)
        message(FATAL_ERROR "You must provide a source AZSL file") 
    endif()

    set(AZSL_path ${CMAKE_CURRENT_LIST_DIR}/${ly_add_shader_AZSL})

    # Send a message to ensure that we are compiling this shader
    if(EXISTS ${AZSL_path})
        message("Fake compiling shader ${ly_add_shader_NAME}")
    else()
        message(FATAL_ERROR "Shader source file ${ly_add_shader_AZSL} doesn't exist")
    endif() 

    # Run the AZSL file through MCPP -----------------------------------------------------------------
    set(MCPP "C:/3rdParty/packages/mcpp-2.7.2_az.1-rev1-windows/mcpp/lib/mcpp.exe")

    set(inc_dirs 
        ${LY_ROOT_FOLDER}/Gems/Atom/RPI/Assets/ShaderLib 
        ${LY_ROOT_FOLDER}/Gems/Atom/Feature/Common/Assets/ShaderLib
        ${LY_ROOT_FOLDER}/Gems/Atom/Feature/Common/Assets/ShaderResourceGroups
    )
    list(TRANSFORM inc_dirs PREPEND "-I")

    # The file to be outputted by MCPP (the flat azsl file)
    set(preprocessed_shader_path ${CMAKE_CURRENT_BINARY_DIR}/Shaders/${ly_add_shader_NAME}.azsl.in)

    # Add the target for this shader
    add_custom_target(${ly_add_shader_NAME} 
        DEPENDS
            ${preprocessed_shader_path}
        SOURCES
            ${AZSL_path}
    )

    # Add the command for this shader
    add_custom_command(
        OUTPUT 
            ${preprocessed_shader_path}
        COMMAND 
            ${MCPP} ${inc_dirs} -C -w -+ ${AZSL_path} -o ${preprocessed_shader_path}
        DEPENDS
            ${AZSL_path}
        COMMAND_EXPAND_LISTS
    )
    
    # Move the target location to inside the designated folder in IDE
    file(RELATIVE_PATH project_path ${LY_ROOT_FOLDER} ${CMAKE_CURRENT_LIST_DIR})
    set(ide_path ${project_path})
    set_target_properties(${ly_add_shader_NAME} PROPERTIES 
        FOLDER "${ide_path}"
    )

endfunction()