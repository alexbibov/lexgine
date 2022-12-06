# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.20)

set(DIRECTXTEX_VERSION 1.9.7)

project (DirectXTex
  VERSION ${DIRECTXTEX_VERSION}
  DESCRIPTION "DirectX Texture Library"
  HOMEPAGE_URL "https://go.microsoft.com/fwlink/?LinkId=248926"
  LANGUAGES CXX)

# Includes the functions for Direct3D 11 resources and DirectCompute compression
option(DirectXTex_BUILD_DX11 "Build with DirectX11 Runtime support" ON)

# Includes the functions for creating Direct3D 12 resources at runtime
option(DirectXTex_BUILD_DX12 "Build with DirectX12 Runtime support" ON)

# Includes the functions for loading/saving OpenEXR files at runtime
# option(ENABLE_OPENEXR_SUPPORT "Build with OpenEXR support" OFF)

if(DEFINED VCPKG_TARGET_ARCHITECTURE)
    set(DIRECTX_ARCH ${VCPKG_TARGET_ARCHITECTURE})
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Ww][Ii][Nn]32$")
    set(DIRECTX_ARCH x86)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Xx]64$")
    set(DIRECTX_ARCH x64)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]$")
    set(DIRECTX_ARCH arm)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]64$")
    set(DIRECTX_ARCH arm64)
endif()

include(GNUInstallDirs)

#--- Library
set(LIBRARY_HEADERS
    DirectXTex.h
    DirectXTex.inl)

set(LIBRARY_SOURCES
    BC.h
    DDS.h
    DirectXTexP.h
    filters.h
    scoped.h
    BC.cpp
    BC4BC5.cpp
    BC6HBC7.cpp
    DirectXTexCompress.cpp
    DirectXTexConvert.cpp
    DirectXTexDDS.cpp
    DirectXTexHDR.cpp
    DirectXTexImage.cpp
    DirectXTexMipmaps.cpp
    DirectXTexMisc.cpp
    DirectXTexNormalMaps.cpp
    DirectXTexPMAlpha.cpp
    DirectXTexResize.cpp
    DirectXTexTGA.cpp
    DirectXTexUtil.cpp)

if(WIN32)
   set(LIBRARY_SOURCES ${LIBRARY_SOURCES}
       DirectXTexFlipRotate.cpp
       DirectXTexWIC.cpp)
endif()

set(SHADER_SOURCES
    Shaders/BC6HEncode.hlsl
    Shaders/BC7Encode.hlsl)

if(DirectXTex_BUILD_DX11 AND WIN32)
   set(LIBRARY_SOURCES ${LIBRARY_SOURCES}
       BCDirectCompute.h
       BCDirectCompute.cpp
       DirectXTexCompressGPU.cpp
       DirectXTexD3D11.cpp)
endif()

if(DirectXTex_BUILD_DX12)
   set(LIBRARY_SOURCES ${LIBRARY_SOURCES}
       d3dx12.h
       DirectXTexD3D12.cpp)
endif()

if(DirectXTex_BUILD_DX11 AND WIN32)
    set(COMPILED_SHADERS ${CMAKE_CURRENT_BINARY_DIR}/Shaders/Compiled)
    set(LIBRARY_SOURCES ${LIBRARY_SOURCES} ${COMPILED_SHADERS}/BC6HEncode_EncodeBlockCS.inc)

    add_custom_command(
      OUTPUT "${COMPILED_SHADERS}/BC6HEncode_EncodeBlockCS.inc"
      MAIN_DEPENDENCY "${PROJECT_SOURCE_DIR}/Shaders/CompileShaders.cmd"
      DEPENDS ${SHADER_SOURCES}
      COMMENT "Generating HLSL shaders..."
      COMMAND ${CMAKE_COMMAND} -E env CompileShadersOutput="${COMPILED_SHADERS}" CompileShaders.cmd > "${COMPILED_SHADERS}/compileshaders.log"
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/Shaders"
      USES_TERMINAL
    )
endif()

add_library(${PROJECT_NAME} STATIC ${LIBRARY_SOURCES} ${LIBRARY_HEADERS})

if(DirectXTex_BUILD_DX11 AND WIN32)
   target_include_directories(${PROJECT_NAME} PRIVATE ${COMPILED_SHADERS})
endif()

source_group(${PROJECT_NAME} REGULAR_EXPRESSION *.*)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)

if(NOT MINGW)
    target_precompile_headers(${PROJECT_NAME} PRIVATE DirectXTexP.h)
endif()

