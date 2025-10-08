cmake_minimum_required(VERSION 3.23)
project(lexgine)

set(ENV{PATH} "${CMAKE_SOURCE_DIR};$ENV{PATH}")

if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
set(_64_bit_build TRUE)
else()
set(_64_bit_build FALSE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(_build_runtime_output_directory ${CMAKE_BINARY_DIR}/bin/)
set(_build_lib_output_directory ${CMAKE_BINARY_DIR}/bin/lib/)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/lib)

get_property(isMultiConfigGenerator GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

function(add_subproject)
    set(options USE_ABSOLUTE_PATH)
    set(oneValueArgs NAME PATH)
    set(multiValueArgs)
    cmake_parse_arguments(PARSE_ARGV 0 add_subproject_arg
        ${options} ${oneValueArgs} ${multiValueArgs}
    )

    if (isMultiConfigGenerator)
        foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
            string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
            set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${_build_runtime_output_directory})
            set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${_build_runtime_output_directory})
            set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${_build_lib_output_directory})
        endforeach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    else(_isMultiConfigGenerator)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${_build_runtime_output_directory})
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${_build_runtime_output_directory})
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${_build_lib_output_directory})
    endif()

    message(STATUS "Adding sub-project ${add_subproject_arg_NAME}")
    if(add_subproject_arg_USE_ABSOLUTE_PATH)
        add_subdirectory(${add_subproject_arg_PATH} ${CMAKE_BINARY_DIR}/${add_subproject_arg_NAME})
    elseif(add_subproject_arg_PATH)
        add_subdirectory(${CMAKE_SOURCE_DIR}/${add_subproject_arg_PATH}/${add_subproject_arg_NAME})
    else()
        add_subdirectory(${CMAKE_SOURCE_DIR}/${add_subproject_arg_NAME})
    endif()
endfunction()

# Download CPM when not preset
set(cpm_download_version 0.42.0)
set(cpm_download_location ${CMAKE_BINARY_DIR}/cmake/CPM_${cpm_download_version}.cmake)
if(NOT EXISTS ${cpm_download_location})
    message(STATUS "Downloading CPM.cmake")
    file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v${cpm_download_version}/CPM.cmake
    ${cpm_download_location})
endif()

# Determine use home directory to store CPM cache)
if(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
  set(user_home "$ENV{HOME}")
elseif(DEFINED ENV{USERPROFILE} AND NOT "$ENV{USERPROFILE}" STREQUAL "")
  set(user_home "$ENV{USERPROFILE}")
elseif(DEFINED ENV{HOMEDRIVE} AND DEFINED ENV{HOMEPATH})
  set(user_home "$ENV{HOMEDRIVE}$ENV{HOMEPATH}")
else()
  message(FATAL_ERROR "Can't determine the user's home directory.")
endif()
cmake_path(APPEND user_home ".CPM" OUTPUT_VARIABLE cpm_cache_location)
file(MAKE_DIRECTORY ${cpm_cache_location})
set(ENV{CPM_SOURCE_CACHE} ${cpm_cache_location})

include(${cpm_download_location})

CPMAddPackage(
    NAME spdlog
    GITHUB_REPOSITORY gabime/spdlog
    VERSION 1.15.3
)
CPMAddPackage(
    NAME glm
    GITHUB_REPOSITORY g-truc/glm
    GIT_TAG 1.0.1
)

######### Configure ImGui source ########
CPMAddPackage(
    NAME imgui
    GITHUB_REPOSITORY ocornut/imgui
    VERSION 1.92.2b
    DOWNLOAD_ONLY TRUE
)
if(imgui_ADDED)
file(
    DOWNLOAD
    "https://gist.githubusercontent.com/rokups/f771217b2d530d170db5cb1e08e9a8f4/raw/4c2c14374ab878ca2f45daabfed4c156468e4e27/CMakeLists.txt"
    "${imgui_SOURCE_DIR}/CMakeLists.txt"
    EXPECTED_HASH SHA256=fd62f69364ce13a4f7633a9b50ae6672c466bcc44be60c69c45c0c6e225bb086
)
set(IMGUI_EXAMPLES FALSE)
set(IMGUI_DEMO FALSE)
set(IMGUI_ENABLE_STDLIB_SUPPORT TRUE)
set(IMGUI_IMPL_SDL FALSE)
set(IMGUI_IMPL_GLUT FALSE)
set(IMGUI_IMPL_GLFW FALSE)
set(IMGUI_IMPL_OPENGL2 FALSE)
set(IMGUI_IMPL_OPENGL3 FALSE)
set(IMGUI_IMPL_VULKAN FALSE)
else()
message(ERROR "Unable to configure ImGui")
endif()
#########################################

CPMAddPackage(
  NAME freetype
  GIT_REPOSITORY https://github.com/freetype/freetype
  GIT_TAG VER-2-14-1
  VERSION 2.14.1
)
if (freetype_ADDED)
  set(FREETYPE_FOUND TRUE)
  set(FREETYPE_INCLUDE_DIRS "")
  set(FREETYPE_LIBRARIES freetype)
  message(STATUS "Found freetype")
endif()

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest
    GIT_TAG v1.17.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_subproject(NAME pugixml PATH 3rd_party)
add_subproject(NAME zlib PATH 3rd_party)
add_subproject(NAME SpookyHash PATH 3rd_party)
add_subproject(NAME ktx PATH 3rd_party)
add_subproject(NAME engine)
add_subproject(NAME common)
add_subproject(NAME imgui PATH ${imgui_SOURCE_DIR} USE_ABSOLUTE_PATH)
add_subproject(NAME texture_converter)
add_subproject(NAME api)
add_subproject(NAME swe_demo PATH demos)
add_subproject(NAME DirectXTex PATH 3rd_party)