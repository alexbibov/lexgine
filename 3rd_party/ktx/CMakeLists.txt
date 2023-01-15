# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.15)

project(ktx DESCRIPTION "Libraries and tools to create and read KTX image texture files.")

# OPTIONS
set( LIB_TYPE_DEFAULT ON )

option( KTX_FEATURE_DOC "Create KTX documentation" OFF )
# option( KTX_FEATURE_STATIC_LIBRARY "Create static libraries (shared otherwise)" ${LIB_TYPE_DEFAULT} )
option( KTX_FEATURE_TESTS "Create unit tests" ON )
option( BASISU_SUPPORT_SSE "Compile with SSE support so applications can choose to use it" ON )

# Might become options in the future

# option( KTX_FEATURE_KTX1 "KTX version 1" ON )
# option( KTX_FEATURE_KTX2 "KTX version 2" ON )

set( KTX_FEATURE_KTX1 ON )
set( KTX_FEATURE_KTX2 ON )
set( KTX_FEATURE_VULKAN ON )

# Platform specific settings
set(bitness 64)
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR FORCE32)
	set(bitness 32)
endif()

# if(KTX_FEATURE_STATIC_LIBRARY)
#     set(LIB_TYPE STATIC)
# else()
#     set(LIB_TYPE SHARED)
# endif()
set(LIB_TYPE STATIC)

# Global Optimization Flags
if(MSVC)
    add_compile_options( $<IF:$<CONFIG:Debug>,/Gz,/O2> )
else()
    add_compile_options( $<IF:$<CONFIG:Debug>,-O0,-O3> )
    add_link_options( $<IF:$<CONFIG:Debug>,-g,-O3> )
endif()

set(KTX_MAIN_SRC
    include/ktx.h
    lib/basis_sgd.h
    lib/basis_transcode.cpp
    lib/basisu/transcoder/basisu_containers.h
    lib/basisu/transcoder/basisu_containers_impl.h
    lib/basisu/transcoder/basisu_file_headers.h
    lib/basisu/transcoder/basisu_global_selector_cb.h
    lib/basisu/transcoder/basisu_global_selector_palette.h
    lib/basisu/transcoder/basisu_transcoder_internal.h
    lib/basisu/transcoder/basisu_transcoder_uastc.h
    lib/basisu/transcoder/basisu_transcoder.cpp
    lib/basisu/transcoder/basisu_transcoder.h
    lib/basisu/transcoder/basisu.h
    lib/basisu/zstd/zstd.c
    lib/checkheader.c
    lib/dfdutils/createdfd.c
    lib/dfdutils/colourspaces.c
    lib/dfdutils/dfd.h
    lib/dfdutils/dfd2vk.inl
    lib/dfdutils/interpretdfd.c
    lib/dfdutils/printdfd.c
    lib/dfdutils/queries.c
    lib/dfdutils/vk2dfd.c
    lib/dfdutils/vulkan/vk_platform.h
    lib/dfdutils/vulkan/vulkan_core.h
    lib/etcdec.cxx
    lib/etcunpack.cxx
    lib/filestream.c
    lib/filestream.h
    lib/formatsize.h
    lib/gl_format.h
    lib/gl_funcs.c
    lib/gl_funcs.h
    lib/glloader.c
    lib/hashlist.c
    lib/info.c
    lib/ktxint.h
    lib/memstream.c
    lib/memstream.h
    lib/stream.h
    lib/strings.c
    lib/swap.c
    lib/texture.c
    lib/texture.h
    lib/texture2.c
    lib/texture2.h
    lib/uthash.h
    lib/vk_format.h
    lib/vkformat_check.c
    lib/vkformat_enum.h
    lib/vkformat_str.c
    )

# Main library
# add_library( ktx ${LIB_TYPE}
#     ${KTX_MAIN_SRC}
#     )

# Read-only library
add_library(ktx_read ${LIB_TYPE} ${KTX_MAIN_SRC})

macro(commom_lib_settings lib write)

    if(TARGET mkvk)
        # Creating vulkan headers is only required when Vulkan SDK updates.
        add_dependencies(${lib} mkvk)
    endif()

    target_compile_definitions(
        ${lib}
    PUBLIC
        "$<$<CONFIG:Debug>:_DEBUG;DEBUG>"
    PRIVATE
        LIBKTX
    )

    # C/C++ Standard
    target_compile_features(${lib} PUBLIC c_std_99 cxx_std_11)

    # Compiler Warning Flags
    target_compile_options(${lib} PRIVATE
            # clang options
            $<$<CXX_COMPILER_ID:AppleClang,Clang>:
                -Wno-nested-anon-types
                -Wno-gnu-anonymous-struct
            >
            # not clang options
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:
                -Wno-pedantic
            >
    )

    target_include_directories(
        ${lib}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/basisu/transcoder
        lib/basisu/transcoder

        ${CMAKE_CURRENT_SOURCE_DIR}/lib/basisu/transcoder
        lib/basisu/zstd

        ${CMAKE_CURRENT_SOURCE_DIR}/other_include
        other_include
    )

    if( LIB_TYPE STREQUAL STATIC )
        target_compile_definitions(${lib} PUBLIC KHRONOS_STATIC)
    endif()

    # BASISD_SUPPORT_KTX2 has to be 1 to compile the encoder. We
    # don't use it. Hopefully it doesn't add too much code. We're using
    # the zstd encoder in basisu by explicitly including the file in our
    # source list. We don't need to related code in the encoder.
    target_compile_definitions(
        ${lib}
    PRIVATE
        BASISD_SUPPORT_KTX2_ZSTD=0
        BASISD_SUPPORT_KTX2=1
    )

    # To reduce size, don't support transcoding to ancient formats.
    target_compile_definitions(${lib} PRIVATE BASISD_SUPPORT_FXT1=0)

    # TODO: make options for all formats and good per-platform defaults
    # - BASISD_SUPPORT_UASTC
    # - BASISD_SUPPORT_DXT1 (BC1)
    # - BASISD_SUPPORT_DXT5A (BC3/4/5)
    # - BASISD_SUPPORT_BC7
    # - BASISD_SUPPORT_BC7_MODE5
    # - BASISD_SUPPORT_PVRTC1
    # - BASISD_SUPPORT_ETC2_EAC_A8
    # - BASISD_SUPPORT_ASTC
    # - BASISD_SUPPORT_ATC
    # - BASISD_SUPPORT_ASTC_HIGHER_OPAQUE_QUALITY
    # - BASISD_SUPPORT_ETC2_EAC_RG11
    # - BASISD_SUPPORT_FXT1
    # - BASISD_SUPPORT_PVRTC2

    if(WIN32 AND NOT (LIB_TYPE STREQUAL STATIC))
        target_compile_definitions(
            ${lib}
        PRIVATE 
            "KTX_API=__declspec(dllexport)"
        PUBLIC # only for basisu_c_binding.
            BASISU_NO_ITERATOR_DEBUG_LEVEL
        )
        target_sources(
            ${lib}
        PRIVATE
            # The msvs generator automatically sets the needed VCLinker
            # option when a .def file is seen in sources.
            lib/internalexport.def
            $<${write}:lib/internalexport_write.def>
        )
    endif()

    if(KTX_FEATURE_KTX1)
        target_compile_definitions(${lib} PUBLIC KTX_FEATURE_KTX1)
        target_sources(
            ${lib}
        PRIVATE
            lib/texture1.c
            lib/texture1.h
        )
    endif()

    if(KTX_FEATURE_KTX2)
        target_compile_definitions(${lib} PUBLIC KTX_FEATURE_KTX2)
    endif()

    if(WIN32)
        # By wrapping in generator expression we force multi configuration generators (like Visual Studio)
        # to take the exact path and not change it.
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${KTX_BUILD_DIR}/$<CONFIG>>)

    elseif(APPLE)
        if(NOT IOS)
            # Set a common RUNTIME_OUTPUT_DIR for all target, so that INSTALL RPATH
            # is functional in build directory as well. BUILD_WITH_INSTALL_RPATH is necessary
            # for working code signing.
            set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${KTX_BUILD_DIR}/$<CONFIG>>)
        endif()

    elseif(LINUX)
        find_package(Threads REQUIRED)
        target_link_libraries(
            ${lib}
        PUBLIC
            dl
            Threads::Threads
        )
    endif()

    if(KTX_FEATURE_VULKAN)
        target_sources(
            ${lib}
        PRIVATE
            include/ktxvulkan.h
            lib/vk_funcs.c
            lib/vk_funcs.h
            lib/vkloader.c
        )
        target_include_directories(
            ${lib}
        PRIVATE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/dfdutils>
            $<INSTALL_INTERFACE:lib/dfdutils>
        )

        get_target_property( KTX_PUBLIC_HEADER ${lib} PUBLIC_HEADER )
        list(APPEND KTX_PUBLIC_HEADER ${CMAKE_SOURCE_DIR}/include/ktxvulkan.h)
        set_target_properties(${lib} PROPERTIES
            PUBLIC_HEADER "${KTX_PUBLIC_HEADER}"
        )
    else()
        target_compile_definitions( ${lib} PRIVATE KTX_OMIT_VULKAN=1 )
    endif()

endmacro(commom_lib_settings)

set(KTX_BUILD_DIR "${CMAKE_BINARY_DIR}")

# commom_lib_settings(ktx 1)
commom_lib_settings(ktx_read 0)


# Adding write capability to target ktx
# target_sources(
#     ktx
# PRIVATE
#     lib/basis_encode.cpp
#     lib/basisu/encoder/apg_bmp.c
#     lib/basisu/encoder/apg_bmp.h
#     lib/basisu/encoder/basisu_astc_decomp.cpp
#     lib/basisu/encoder/basisu_astc_decomp.h
#     lib/basisu/encoder/basisu_backend.cpp
#     lib/basisu/encoder/basisu_backend.h
#     lib/basisu/encoder/basisu_basis_file.cpp
#     lib/basisu/encoder/basisu_basis_file.h
#     lib/basisu/encoder/basisu_bc7enc.cpp
#     lib/basisu/encoder/basisu_bc7enc.h
#     lib/basisu/encoder/basisu_comp.cpp
#     lib/basisu/encoder/basisu_comp.h
#     lib/basisu/encoder/basisu_enc.cpp
#     lib/basisu/encoder/basisu_enc.h
#     lib/basisu/encoder/basisu_etc.cpp
#     lib/basisu/encoder/basisu_etc.h
#     lib/basisu/encoder/basisu_frontend.cpp
#     lib/basisu/encoder/basisu_frontend.h
#     lib/basisu/encoder/basisu_global_selector_palette_helpers.cpp
#     lib/basisu/encoder/basisu_global_selector_palette_helpers.h
#     lib/basisu/encoder/basisu_gpu_texture.cpp
#     lib/basisu/encoder/basisu_gpu_texture.h
#     lib/basisu/encoder/basisu_kernels_declares.h
#     lib/basisu/encoder/basisu_kernels_imp.h
#     lib/basisu/encoder/basisu_kernels_sse.cpp
#     lib/basisu/encoder/basisu_miniz.h
#     lib/basisu/encoder/basisu_pvrtc1_4.cpp
#     lib/basisu/encoder/basisu_pvrtc1_4.h
#     lib/basisu/encoder/basisu_resample_filters.cpp
#     lib/basisu/encoder/basisu_resampler_filters.h
#     lib/basisu/encoder/basisu_resampler.cpp
#     lib/basisu/encoder/basisu_resampler.h
#     lib/basisu/encoder/basisu_ssim.cpp
#     lib/basisu/encoder/basisu_ssim.h
#     lib/basisu/encoder/basisu_uastc_enc.cpp
#     lib/basisu/encoder/basisu_uastc_enc.h
#     lib/basisu/encoder/cppspmd_flow.h
#     lib/basisu/encoder/cppspmd_math.h
#     lib/basisu/encoder/cppspmd_math_declares.h
#     lib/basisu/encoder/cppspmd_sse.h
#     lib/basisu/encoder/cppspmd_type_aliases.h
#     lib/basisu/encoder/jpgd.cpp
#     lib/basisu/encoder/jpgd.h
#     lib/basisu/encoder/lodepng.cpp
#     lib/basisu/encoder/lodepng.h
#     lib/writer1.c
#     lib/writer2.c
# )
# target_include_directories(
#     ktx
# PRIVATE
#     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/basisu>
#     $<INSTALL_INTERFACE:lib/basisu>
# )
# target_compile_definitions(
#     ktx
# PUBLIC
#     KTX_FEATURE_WRITE
# )
# if(BASISU_SUPPORT_SSE)
#     target_compile_definitions(
#         ktx
#     PRIVATE
#         BASISU_SUPPORT_SSE=1
#     )
#     target_compile_options(
#         ktx
#     PRIVATE
#         $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:
#             -msse4.1
#         >
#     )
# else()
#     target_compile_definitions(
#         ktx
#     PRIVATE
#         BASISU_SUPPORT_SSE=0
#     )
# endif()