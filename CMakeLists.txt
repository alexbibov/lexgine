cmake_minimum_required(VERSION 3.17)
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

function(add_subproject name)
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

    if(${ARGC} EQUAL 2)
        add_subdirectory(${CMAKE_SOURCE_DIR}/${ARGV1}/${name})
    else(${ARGC} EQUAL 1)
        add_subdirectory(${CMAKE_SOURCE_DIR}/${name})
    endif()
endfunction()

include(FetchContent)
FetchContent_Declare(
    googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG release-1.12.1
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_subproject(glm 3rd_party)
add_subproject(imgui 3rd_party)
add_subproject(pugixml 3rd_party)
add_subproject(zlib 3rd_party)
add_subproject(lua 3rd_party)
add_subproject(SpookyHash 3rd_party)
add_subproject(engine)
add_subproject(common)
add_subproject(texture_converter)
add_subproject(api)
add_subproject(swe_demo demos)
