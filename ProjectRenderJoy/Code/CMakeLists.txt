
# Currently we are in the ProjectRenderJoy/Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: ly_get_list_relative_pal_filename will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here : ProjectRenderJoy/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/ProjectRenderJoy/Code
ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME})

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this project
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Now that we have loaded our project traits for this platform, see if this project is even supported on this platform.
# If its not supported we just return after including the unsupported.
if(NOT PAL_TRAIT_PROJECTRENDERJOY_SUPPORTED)
    return()
endif()

# We are on a supported platform, so add the ProjectRenderJoy target
# Note: We include the common files and the platform specific files which are set in projectrenderjoy_files.cmake and
# in ${pal_dir}/projectrenderjoy_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
ly_add_target(
    NAME ProjectRenderJoy.Static STATIC
    NAMESPACE Project
    FILES_CMAKE
        projectrenderjoy_files.cmake
        ${pal_dir}/projectrenderjoy_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            AZ::AzGameFramework
            Gem::Atom_AtomBridge.Static
)

ly_add_target(
    NAME ProjectRenderJoy ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Project
    FILES_CMAKE
        projectrenderjoy_shared_files.cmake
        ${pal_dir}/projectrenderjoy_shared_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Project::ProjectRenderJoy.Static
            AZ::AzCore
)

################################################################################
# Gem dependencies
################################################################################
ly_add_project_dependencies(
    PROJECT_NAME
        ProjectRenderJoy
    TARGETS 
        ProjectRenderJoy.GameLauncher
    DEPENDENCIES_FILES
        runtime_dependencies.cmake
        ${pal_dir}/${PAL_PLATFORM_NAME_LOWERCASE}_runtime_dependencies.cmake
)

if(PAL_TRAIT_BUILD_HOST_TOOLS)
    ly_add_project_dependencies(
        PROJECT_NAME
            ProjectRenderJoy
        TARGETS
            AssetBuilder
            AssetProcessor
            AssetProcessorBatch
            Editor
        DEPENDENCIES_FILES
            tool_dependencies.cmake
            ${pal_dir}/${PAL_PLATFORM_NAME_LOWERCASE}_tool_dependencies.cmake
    )
endif()

if(PAL_TRAIT_BUILD_SERVER_SUPPORTED)
    ly_add_project_dependencies(
        PROJECT_NAME
            ProjectRenderJoy
        TARGETS
            ProjectRenderJoy.ServerLauncher
        DEPENDENCIES_FILES
            server_dependencies.cmake
            ${pal_dir}/${PAL_PLATFORM_NAME_LOWERCASE}_server_dependencies.cmake
    )
    set_property(GLOBAL APPEND PROPERTY LY_LAUNCHER_SERVER_PROJECTS ProjectRenderJoy)

endif()
