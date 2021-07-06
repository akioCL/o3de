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
# set(mcpp_inc_dirs "")
set_property(GLOBAL PROPERTY mcpp_inc_dirs)

#! ly_add_shader: adds a shader to be compiled
function(ly_add_shader)
    
    # Parse the arguments for the function calls ------------------------------------------------------
    set(options)
    set(oneValueArgs NAME AZSL SHADER_VARIANT_LIST ENTRY_VS ENTRY_PS)
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

    set(SVL_path ${CMAKE_CURRENT_LIST_DIR}/${ly_add_shader_SHADER_VARIANT_LIST})
    if(NOT EXISTS ${SVL_path})
        message(FATAL_ERROR "Shader variant list file ${ly_add_shader_SHADER_VARIANT_LIST} doesn't exist")
    endif()

    # Parse the shadervariantlist file -----------------------------------------------------------------
    file(READ ${SVL_path} svl_json_string)
    string(JSON variants GET ${svl_json_string} "Variants")
    string(JSON num_variants ERROR_VARIABLE json_error
        LENGTH ${svl_json_string} "Variants")
    math(EXPR variants_range "${num_variants} - 1")

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
    endforeach()

    # Get the output paths for each RHI & shader variant
    foreach(rhi ${RHI_names})
        foreach(val RANGE ${variants_range})
            string(JSON variant GET ${variants} "${val}")
            string(JSON id GET ${variant} "StableId")
            list(APPEND final_hlsl_paths ${CMAKE_CURRENT_BINARY_DIR}/Shaders/final_hlsl/${ly_add_shader_NAME}-${id}.${rhi}.hlsl)
            list(APPEND hlsl_output_paths ${CMAKE_CURRENT_BINARY_DIR}/Shaders/azslc_outputs/${ly_add_shader_NAME}-${id}.${rhi}.hlsl)
            list(APPEND dxc_output_e1_paths ${CMAKE_CURRENT_BINARY_DIR}/Shaders/dxc_outputs/${ly_add_shader_NAME}-${id}-e1.${rhi}.dxil.bin)
            list(APPEND dxc_output_e2_paths ${CMAKE_CURRENT_BINARY_DIR}/Shaders/dxc_outputs/${ly_add_shader_NAME}-${id}-e2.${rhi}.dxil.bin)
        endforeach()
    endforeach()

    # Set up MCPP, AZSLc, and DXC -----------------------------------------------------------------
    get_property(inc_dirs GLOBAL PROPERTY mcpp_inc_dirs)
    if (NOT inc_dirs)
        ly_add_shader_atom_include_dirs()
        get_property(inc_dirs GLOBAL PROPERTY mcpp_inc_dirs)
    endif()

    set(MCPP "C:/3rdParty/packages/mcpp-2.7.2_az.1-rev1-windows/mcpp/lib/mcpp.exe")
    set(AZSLC "C:/3rdParty/packages/azslc-1.7.21-rev1-multiplatform/azslc/bin/win_x64/Release/azslc.exe")
    set(DXC "C:/3rdParty/packages/DirectXShaderCompilerDxc-1.6.2104-o3de-rev2-windows/DirectXShaderCompilerDxc/bin/Release/dxc.exe")

    # Add the target for this shader -----------------------------------------------------------------
    add_custom_target(${ly_add_shader_NAME} 
        DEPENDS
            ${dxc_output_e2_paths}
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
        set(cat_rhi_source_files
            ${rhi_path}
            ${AZSL_path}
        )

        # Concatenate the RHI AZSLC header to the AZSL file
        execute_process(COMMAND ${CMAKE_COMMAND} -E cat ${cat_rhi_source_files}
	        OUTPUT_VARIABLE concat_files 
	        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Shaders
        )
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/Shaders/prepend/${ly_add_shader_NAME}.${rhi_name}.prepend.in "${concat_files}" )
        configure_file(${CMAKE_CURRENT_BINARY_DIR}/Shaders/prepend/${ly_add_shader_NAME}.${rhi_name}.prepend.in ${CMAKE_CURRENT_BINARY_DIR}/Shaders/prepend/${ly_add_shader_NAME}.${rhi_name}.prepend COPYONLY)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Shaders/prepend/${ly_add_shader_NAME}.${rhi_name}.prepend
	        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/prepend/Shaders)
        set(cat_AZSL_rhi_path ${CMAKE_CURRENT_BINARY_DIR}/Shaders/prepend/${ly_add_shader_NAME}.${rhi_name}.prepend)

        # Make list of names for each supervariant for this RHI
        foreach(num_variant RANGE ${variants_range})
            string(JSON variant GET ${variants} "${num_variant}")
            string(JSON shader_options GET ${variant} "Options")
            string(JSON id GET ${variant} "StableId")

            # Parse the shader options 
            set(shader_string)
            string(JSON num_options ERROR_VARIABLE json_error
                LENGTH ${variant} "Options")
            math(EXPR options_range "${num_options} - 1")
            foreach(num_option RANGE ${options_range})
                string(JSON option_name MEMBER ${shader_options} ${num_option})
                string(JSON option_value GET ${shader_options} ${option_name})
                set(shader_string "${shader_string}${option_name}=${option_value};")
            endforeach()

            math(EXPR idx "${val} * ${num_variants} + ${num_variant}")
            list(GET hlsl_output_paths ${idx} output_hlsl_path)
            list(GET dxc_output_e1_paths ${idx} dxc_output_e1_path)
            list(GET dxc_output_e2_paths ${idx} dxc_output_e2_path)

            # Run the azsl file through MCPP and AZSLC to output the hlsl file + other outputs
            add_custom_command(
                OUTPUT 
                    ${output_hlsl_path}
                COMMAND 
                    ${MCPP} ${inc_dirs} -C -w -+ ${cat_AZSL_rhi_path} -o ${preprocessed_shader_path}
                COMMAND 
                    ${AZSLC} ${preprocessed_shader_path} --full -o ${output_hlsl_path}
                DEPENDS
                    ${cat_rhi_source_files}
                COMMAND_EXPAND_LISTS
            )

            set(final_hlsl_path ${CMAKE_CURRENT_BINARY_DIR}/Shaders/final_hlsl/${ly_add_shader_NAME}-${id}.${rhi_name}.hlsl)
            add_custom_command( 
                OUTPUT 
                    ${dxc_output_e2_path}
                COMMAND 
                    "python" "${LY_ROOT_FOLDER}/cmake/tools/shader_variant_tool.py" "${ly_add_shader_NAME}-${id}.${rhi_name}" "${shader_string}" "${CMAKE_CURRENT_BINARY_DIR}" "${rhi_hlsl_header_path}"
                COMMAND 
                    ${DXC} ${final_hlsl_path} -T "ps_6_0" -E ${ly_add_shader_ENTRY_PS} -Fo ${dxc_output_e1_path}
                COMMAND 
                    ${DXC} ${final_hlsl_path} -T "vs_6_0" -E ${ly_add_shader_ENTRY_VS} -Fo ${dxc_output_e2_path}
                DEPENDS 
                    ${output_hlsl_path}
                    "${LY_ROOT_FOLDER}/cmake/tools/shader_variant_tool.py" # need to fix this (will note rebuild for this :0)
            )

        endforeach()
    endforeach()

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