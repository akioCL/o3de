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

# Include directories to be used for MCPP
# Can be edited via
#    ly_add_shader_include_dirs(), which takes directories and adds them
#    ly_add_shader_atom_include_dirs(), which adds all the Atom included directories
set_property(GLOBAL PROPERTY mcpp_inc_dirs)

#! ly_add_shader: adds a shader to be compiled
function(ly_add_shader)
    
    # Parse the arguments for the function calls ------------------------------------------------------
    set(options)
    set(oneValueArgs NAME AZSL SHADER_VARIANTS ENTRY_VS ENTRY_PS)
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
    if(NOT EXISTS ${AZSL_path})
        message(FATAL_ERROR "Shader source file ${ly_add_shader_AZSL} doesn't exist")
    endif() 

    set(sv_path ${CMAKE_CURRENT_LIST_DIR}/${ly_add_shader_SHADER_VARIANTS})
    if(NOT EXISTS ${sv_path})
        message(FATAL_ERROR "Shader variants file ${sv_path} doesn't exist")
    endif()

    # Set up RHI versions -----------------------------------------------------------------
    set(RHI_names
        Android_Vulkan
        iOS_Metal
        Mac_Metal
        Mac_Null
        Windows_DX12
        Windows_Null
        Windows_Vulkan
    )
    set(RHI_azslc_header_paths
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Android/Vulkan/AzslcHeader.azsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/iOS/Metal/AzslcHeader.azsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Mac/Metal/AzslcHeader.azsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Mac/Null/AzslcHeader.azsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Windows/DX12/AzslcHeader.azsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Windows/Null/AzslcHeader.azsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Windows/Vulkan/AzslcHeader.azsli
    )
    set(RHI_hlsl_header_paths
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Android/Vulkan/PlatformHeader.hlsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/iOS/Metal/PlatformHeader.hlsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Mac/Metal/PlatformHeader.hlsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Mac/Null/PlatformHeader.hlsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Windows/DX12/PlatformHeader.hlsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Windows/Null/PlatformHeader.hlsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Windows/Vulkan/PlatformHeader.hlsli
    )
    foreach(rhi ${RHI_names})
        list(APPEND RHI_output_paths ${CMAKE_CURRENT_BINARY_DIR}/Shaders/${ly_add_shader_NAME}.${rhi}.azsl.in)
        list(APPEND hlsl_output_paths ${CMAKE_CURRENT_BINARY_DIR}/Shaders/azslc_outputs/${ly_add_shader_NAME}.${rhi}.hlsl)
    endforeach()

    # Set up MCPP, AZSLc, and DXC -----------------------------------------------------------------
    get_property(inc_dirs GLOBAL PROPERTY mcpp_inc_dirs)
    if (NOT inc_dirs)
        ly_add_shader_atom_include_dirs()
        get_property(inc_dirs GLOBAL PROPERTY mcpp_inc_dirs)
    endif()

    set(MCPP "C:/3rdParty/packages/mcpp-2.7.2_az.1-rev1-windows/mcpp/lib/mcpp.exe")
    set(AZSLC "C:/3rdParty/packages/azslc-1.7.21-rev1-multiplatform/azslc/bin/win_x64/Release/azslc.exe")

    # Add the target for this shader -----------------------------------------------------------------
    add_custom_target(${ly_add_shader_NAME} 
        DEPENDS
            ${hlsl_output_paths}
        SOURCES
            ${AZSL_path}
    )

    # Add the custom commands for this shader ---------------------------------------------------------------
    # 1. For each RHI, concatenate the header RHI file to the AZSL file
    # 2. Run the concatenated file through MCPP & AZSLc
    # 3. For each shader variant, create the variant header and concatenate it to the outputted HLSL file from AZSLc
    # 4. Run the final HLSL files through DXC for each entry point
    list(LENGTH RHI_names len1) 
    math(EXPR len "${len1} - 1")
    foreach(val RANGE ${len})
        list(GET RHI_output_paths ${val} preprocessed_shader_path)
        list(GET RHI_azslc_header_paths ${val} rhi_path)
        list(GET RHI_names ${val} rhi_name)
        list(GET RHI_hlsl_header_paths ${val} rhi_hlsl_header_path)
        list(GET hlsl_output_paths ${val} output_hlsl_path)
        set(cat_rhi_source_files
            ${rhi_path}
            ${AZSL_path}
        )
        
        set(cat_AZSL_rhi_path ${CMAKE_CURRENT_BINARY_DIR}/Shaders/prepend/${ly_add_shader_NAME}.${rhi_name}.prepend)

        # Run the azsl file through MCPP and AZSLC to output the hlsl file + other outputs
        add_custom_command(
            OUTPUT 
                ${output_hlsl_path}
            COMMAND 
                python "${LY_ROOT_FOLDER}/cmake/tools/shader_variant_tool.py" "concat" ${CMAKE_CURRENT_BINARY_DIR} ${rhi_path} ${AZSL_path} ${cat_AZSL_rhi_path}
            COMMAND 
                ${MCPP} ${inc_dirs} -C -w -+ ${cat_AZSL_rhi_path} -o ${preprocessed_shader_path}
            COMMAND 
                ${AZSLC} ${preprocessed_shader_path} --full -o ${output_hlsl_path}
            DEPENDS
                ${cat_rhi_source_files}
            COMMAND_EXPAND_LISTS
            VERBATIM
        )
    endforeach() 

    include(${sv_path})

    # Move the target location to inside the designated folder in IDE
    file(RELATIVE_PATH project_path ${LY_ROOT_FOLDER} ${CMAKE_CURRENT_LIST_DIR})
    set(ide_path ${project_path})
    set_target_properties(${ly_add_shader_NAME} PROPERTIES 
        FOLDER "${ide_path}"
    )
endfunction()

# Add the Atom included shader directories to inc_dirs
function(ly_add_shader_atom_include_dirs)
    set(atom_inc_dirs 
        ${LY_ROOT_FOLDER}/Gems/Atom/RPI/Assets/ShaderLib 
        ${LY_ROOT_FOLDER}/Gems/Atom/Feature/Common/Assets/ShaderLib
        ${LY_ROOT_FOLDER}/Gems/Atom/Feature/Common/Assets/ShaderResourceGroups
        ${LY_ROOT_FOLDER}/Gems
        ${LY_ROOT_FOLDER}/Gems/Atom/Feature/Common/Assets/Materials/Types
    )
    list(TRANSFORM atom_inc_dirs PREPEND "-I")
    get_property(temp GLOBAL PROPERTY mcpp_inc_dirs)
    set(temp ${temp}
        ${atom_inc_dirs})
    set_property(GLOBAL PROPERTY mcpp_inc_dirs "${temp}")
endfunction()

# Add specific directories to inc_dirs
function(ly_add_shader_include_dirs)
    set(dirs ${ARGV})
    list(TRANSFORM dirs PREPEND "-I${CMAKE_CURRENT_LIST_DIR}/") 
    get_property(temp GLOBAL PROPERTY mcpp_inc_dirs)
    set(temp ${temp}
        ${dirs})
    set_property(GLOBAL PROPERTY mcpp_inc_dirs "${temp}")
endfunction()

function(ly_add_shader_variant)
    # Parse the arguments for the function calls ------------------------------------------------------
    set(options)
    set(oneValueArgs NAME STABLE_ID ENTRY_VS ENTRY_PS)
    set(multiValueArgs OPTIONS ROOT_FILES)

    cmake_parse_arguments(ly_add_shader_variant "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate input arguments ------------------------------------------------------------------------
    if(NOT ly_add_shader_variant_NAME)
        message(FATAL_ERROR "You must provide a name for the target")
    endif()
    if(NOT ly_add_shader_variant_STABLE_ID)
        message(FATAL_ERROR "You must provide a stable id for this varaint") 
    endif()

    if(NOT ly_add_shader_variant_OPTIONS)
        set(ly_add_shader_variant_OPTIONS " ")
    endif()

    #[[
    foreach(root_file ${ly_add_shader_variant_ROOT_FILES})
        if(NOT EXISTS root_file)
            message(FATAL_ERROR "Must have the HLSL files (run the root target first)")
        endif() 
    endforeach()]]

    set(variant_name ${ly_add_shader_variant_NAME}-${ly_add_shader_variant_STABLE_ID})
    set(sv_list_path ${CMAKE_CURRENT_BINARY_DIR}/Shaders/shader_variant_lists)
    
    execute_process(
        COMMAND 
            python "${LY_ROOT_FOLDER}/cmake/tools/shader_variant_tool.py" "add_json" ${ly_add_shader_variant_NAME} ${ly_add_shader_variant_STABLE_ID} ${ly_add_shader_variant_OPTIONS} ${sv_list_path})

    # Set up DXC + inputs/outputs -----------------------------------------------------------------
    set(DXC "C:/3rdParty/packages/DirectXShaderCompilerDxc-1.6.2104-o3de-rev2-windows/DirectXShaderCompilerDxc/bin/Release/dxc.exe")
    set(RHI_names
        Android_Vulkan
        iOS_Metal
        Mac_Metal
        Mac_Null
        Windows_DX12
        Windows_Null
        Windows_Vulkan
    )
    set(RHI_hlsl_header_paths
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Android/Vulkan/PlatformHeader.hlsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/iOS/Metal/PlatformHeader.hlsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Mac/Metal/PlatformHeader.hlsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Mac/Null/PlatformHeader.hlsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Windows/DX12/PlatformHeader.hlsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Windows/Null/PlatformHeader.hlsli
        ${LY_ROOT_FOLDER}/Gems/Atom/Asset/Shader/Code/AZSL/Platform/Windows/Vulkan/PlatformHeader.hlsli
    )
    foreach(rhi ${RHI_names})
        list(APPEND dxc_outputs_e1 ${CMAKE_CURRENT_BINARY_DIR}/Shaders/dxc_outputs/${variant_name}-e1.${rhi}.dxil.bin)
        list(APPEND dxc_outputs_e2 ${CMAKE_CURRENT_BINARY_DIR}/Shaders/dxc_outputs/${variant_name}-e2.${rhi}.dxil.bin)
        list(APPEND final_hlsl_paths ${CMAKE_CURRENT_BINARY_DIR}/Shaders/final_hlsl/${variant_name}.${rhi}.hlsl)
    endforeach()

    # Create a target for this variant
    add_custom_target(${variant_name}
        DEPENDS
            ${dxc_outputs_e1}
            ${dxc_outputs_e2}
    )
    add_dependencies(${variant_name} ${ly_add_shader_variant_NAME})

    list(LENGTH ly_add_shader_variant_ROOT_FILES len1) 
    math(EXPR len "${len1} - 1")
    foreach(val RANGE ${len}) 
        list(GET RHI_names ${val} rhi_name)
        list(GET ly_add_shader_variant_ROOT_FILES ${val} root_hlsl_file)
        list(GET RHI_hlsl_header_paths ${val} rhi_hlsl_header_path)
        list(GET final_hlsl_paths ${val} final_hlsl_path)
        list(GET dxc_outputs_e1 ${val} dxc_output_e1_path)
        list(GET dxc_outputs_e2 ${val} dxc_output_e2_path)
        add_custom_command( 
            OUTPUT 
                ${dxc_output_e1_path}
                ${dxc_output_e2_path}
            COMMAND 
                "python" "${LY_ROOT_FOLDER}/cmake/tools/shader_variant_tool.py" "main" "${variant_name}.${rhi_name}" "${ly_add_shader_variant_NAME}.${rhi_name}" "${ly_add_shader_variant_OPTIONS}" "${CMAKE_CURRENT_BINARY_DIR}" "${rhi_hlsl_header_path}"
            COMMAND 
                ${DXC} ${final_hlsl_path} -T "ps_6_2" -E ${ly_add_shader_variant_ENTRY_PS} -Fo ${dxc_output_e1_path}
            COMMAND 
                ${DXC} ${final_hlsl_path} -T "vs_6_2" -E ${ly_add_shader_variant_ENTRY_VS} -Fo ${dxc_output_e2_path}
            DEPENDS 
                ${root_hlsl_file}
                ${rhi_hlsl_header_path}
        )
    endforeach()

    # Move the target location to inside the designated folder in IDE
    file(RELATIVE_PATH project_path ${LY_ROOT_FOLDER} ${CMAKE_CURRENT_LIST_DIR})
    set(ide_path ${project_path}/${ly_add_shader_variant_NAME})
    set_target_properties(${variant_name} PROPERTIES 
        FOLDER "${ide_path}"
    )
endfunction()