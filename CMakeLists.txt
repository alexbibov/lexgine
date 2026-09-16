cmake_minimum_required(VERSION 3.23)
project(lexgine)

set(ENV{PATH} "${CMAKE_SOURCE_DIR};$ENV{PATH}")

if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
set(_64_bit_build TRUE)
else()
set(_64_bit_build FALSE)
message(FATAL_ERROR
"lexgine requires a 64-bit toolchain, but ${CMAKE_CXX_COMPILER} targets a "
"${CMAKE_SIZEOF_VOID_P}-byte pointer architecture. For the Ninja presets the target "
"architecture comes from the environment, so select an amd64 (or arm64) toolchain in "
"the IDE -- in CLion under Settings | Build, Execution, Deployment | Toolchains | "
"Architecture.")
endif()

if(NOT MSVC)
message(FATAL_ERROR
"lexgine requires an MSVC-compatible toolchain (cl.exe or clang-cl.exe), but the "
"configured C++ compiler is ${CMAKE_CXX_COMPILER_ID} (${CMAKE_CXX_COMPILER}). The engine "
"targets Direct3D 12 and links the x64 dxcompiler and WinPixEventRuntime import "
"libraries, neither of which is consumable by GCC/MinGW. Select a Visual Studio "
"toolchain -- in CLion under Settings | Build, Execution, Deployment | Toolchains, where "
"it must also be the toolchain of the CMake profile.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(_build_output_root ${CMAKE_BINARY_DIR}/bin)

get_property(isMultiConfigGenerator GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if (isMultiConfigGenerator)
    set(_build_config_list ${CMAKE_CONFIGURATION_TYPES})
else()
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build configuration of a single-configuration generator" FORCE)
    endif()
    set(_build_config_list ${CMAKE_BUILD_TYPE})
endif()

# Every configuration owns a separate bin/<Config> sub-tree so that artifacts of
# different configurations never overwrite each other, which previously happened
# for multi-configuration generators emitting all configurations into bin/.
# These two variables carry a $<CONFIG> generator expression and hence are only
# usable in target_* commands; use _build_output_root together with
# _build_config_list where a configure-time path is required.
set(_build_runtime_output_directory ${_build_output_root}/$<CONFIG>)
set(_build_lib_output_directory ${_build_output_root}/$<CONFIG>/lib)

foreach(config ${_build_config_list})
    file(MAKE_DIRECTORY ${_build_output_root}/${config}/lib)
endforeach()

# Sets the artifact output directories of the current scope. Invoked at top
# scope, before any dependency is declared, so that every target -- including
# CPM/FetchContent third-party deps (freetype, blake3, xxhash, googletest, ...)
# -- emits its runtime artifacts into bin/<Config>. Otherwise, with
# BUILD_SHARED_LIBS=ON those deps build DLLs that land in their own
# _deps/<name>-build/<config>/ folders, and the test/app executables in
# bin/<Config> fail to load them at runtime (exit 127 / gtest_discover_tests
# MSB3073).
macro(set_output_directories)
    if (isMultiConfigGenerator)
        foreach(_config ${CMAKE_CONFIGURATION_TYPES})
            string(TOUPPER ${_config} _config_uppercase)
            set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_config_uppercase} ${_build_output_root}/${_config})
            set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${_config_uppercase} ${_build_output_root}/${_config})
            set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${_config_uppercase} ${_build_output_root}/${_config}/lib)
        endforeach()
    else()
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${_build_output_root}/${CMAKE_BUILD_TYPE})
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${_build_output_root}/${CMAKE_BUILD_TYPE})
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${_build_output_root}/${CMAKE_BUILD_TYPE}/lib)
    endif()
endmacro()

set_output_directories()

function(add_subproject)
    set(options USE_ABSOLUTE_PATH)
    set(oneValueArgs NAME PATH)
    set(multiValueArgs)
    cmake_parse_arguments(PARSE_ARGV 0 add_subproject_arg
        ${options} ${oneValueArgs} ${multiValueArgs}
    )

    set_output_directories()

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

# Build spdlog as a static library (despite the project-wide BUILD_SHARED_LIBS=ON)
# so its compiled code is linked into lexgine.dll instead of producing a separate
# spdlog(d).dll. A normal variable shadows the cached BUILD_SHARED_LIBS for
# spdlog's add_library() call; unset afterwards to restore shared builds.
set(BUILD_SHARED_LIBS OFF)
CPMAddPackage(
    NAME spdlog
    GITHUB_REPOSITORY gabime/spdlog
    VERSION 1.17.0
)
unset(BUILD_SHARED_LIBS)

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

# Build BLAKE3 as a static library so it is linked into lexgine.dll (via common)
# instead of producing a separate blake3.dll. See the spdlog note above.
set(BUILD_SHARED_LIBS OFF)
CPMAddPackage(
  NAME BLAKE-3
  GITHUB_REPOSITORY BLAKE3-team/BLAKE3
  GIT_TAG 1.8.5
  SOURCE_SUBDIR c
)
unset(BUILD_SHARED_LIBS)

set(XXHASH_BUILD_XXHSUM OFF CACHE BOOL "" FORCE)
CPMAddPackage(
  NAME xxHash
  GITHUB_REPOSITORY Cyan4973/xxHash
  GIT_TAG v0.8.3
  SOURCE_SUBDIR cmake_unofficial
)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest
    GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(googletest)
unset(BUILD_SHARED_LIBS)

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
