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

    # Run the AZSL file through MCPP & AZSLc -----------------------------------------------------------------
    set(MCPP "C:/3rdParty/packages/mcpp-2.7.2_az.1-rev1-windows/mcpp/lib/mcpp.exe")

    set(inc_dirs 
        ${LY_ROOT_FOLDER}/Gems/Atom/RPI/Assets/ShaderLib 
        ${LY_ROOT_FOLDER}/Gems/Atom/Feature/Common/Assets/ShaderLib
        ${LY_ROOT_FOLDER}/Gems/Atom/Feature/Common/Assets/ShaderResourceGroups
        ${LY_ROOT_FOLDER}/AtomSampleViewer/ShaderLib
        ${LY_ROOT_FOLDER}/Gems
        ${LY_ROOT_FOLDER}/AtomSampleViewer
        ${LY_ROOT_FOLDER}/Gems/Atom/Feature/Common/Assets/Materials/Types
    )
    list(TRANSFORM inc_dirs PREPEND "-I")

    set(RHI_names
        Android_Vulkan
        iOS_Metal
        Mac_Metal
        Mac_Null
        Windows_DX12
        Windows_Null
        Windows_Vulkan
    )

    set(RHI_paths
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Android/Vulkan/AzslcHeader.azsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/iOS/Metal/AzslcHeader.azsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Mac/Metal/AzslcHeader.azsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Mac/Null/AzslcHeader.azsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Windows/DX12/AzslcHeader.azsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Windows/Null/AzslcHeader.azsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Windows/Vulkan/AzslcHeader.azsli
    )

    foreach(rhi ${RHI_names})
        list(APPEND RHI_output_paths ${CMAKE_CURRENT_BINARY_DIR}/Shaders/${ly_add_shader_NAME}.${rhi}.azsl.in)
    endforeach()

    # Add the target for this shader
    add_custom_target(${ly_add_shader_NAME} 
        DEPENDS
            ${RHI_output_paths}
        SOURCES
            ${AZSL_path}
    )

    list(LENGTH RHI_names len1) 
    math(EXPR len "${len1} - 1")     
    # For each RHI, concatenate the header RHI file to the AZSL file and run the file through MCPP
    foreach(val RANGE ${len})
        list(GET RHI_output_paths ${val} preprocessed_shader_path)
        list(GET RHI_paths ${val} rhi_path)
        list(GET RHI_names ${val} rhi_name)
        # The file to be outputted by MCPP (the flat azsl file)
        # set(preprocessed_shader_path ${CMAKE_CURRENT_BINARY_DIR}/Shaders/${ly_add_shader_NAME}.${rhi_name}.azsl.in)

        set(cat_rhi_source_files
            ${rhi_path}
            ${AZSL_path}
        )

        # Concatenate the files together
        execute_process(COMMAND ${CMAKE_COMMAND} -E cat ${cat_rhi_source_files}
	        OUTPUT_VARIABLE concat_files 
	        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Shaders
        )
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/Shaders/prepend/${ly_add_shader_NAME}.${rhi_name}.prepend.in "${concat_files}" )
        configure_file(${CMAKE_CURRENT_BINARY_DIR}/Shaders/prepend/${ly_add_shader_NAME}.${rhi_name}.prepend.in ${CMAKE_CURRENT_BINARY_DIR}/Shaders/prepend/${ly_add_shader_NAME}.${rhi_name}.prepend COPYONLY)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Shaders/prepend/${ly_add_shader_NAME}.${rhi_name}.prepend
	        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/prepend/Shaders)
        
        set(cat_AZSL_rhi_path ${CMAKE_CURRENT_BINARY_DIR}/Shaders/prepend/${ly_add_shader_NAME}.${rhi_name}.prepend)

        add_custom_command(
            OUTPUT 
                ${preprocessed_shader_path}
            COMMAND 
                ${MCPP} ${inc_dirs} -C -w -+ ${cat_AZSL_rhi_path} -o ${preprocessed_shader_path}
            DEPENDS
                ${cat_rhi_source_files}
            COMMAND_EXPAND_LISTS
        )
    endforeach()

    # Move the target location to inside the designated folder in IDE
    file(RELATIVE_PATH project_path ${LY_ROOT_FOLDER} ${CMAKE_CURRENT_LIST_DIR})
    set(ide_path ${project_path})
    set_target_properties(${ly_add_shader_NAME} PROPERTIES 
        FOLDER "${ide_path}"
    )

endfunction()