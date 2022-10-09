cmake_minimum_required(VERSION 3.17)
set(_project_name engine)
project(${_project_name})

set(_src_root_path ${PROJECT_SOURCE_DIR})
set(_target ${_project_name})

file(
    GLOB_RECURSE _sources_list
    CONFIGURE_DEPENDS
    "${_src_root_path}/*.c" "${_src_root_path}/*.cpp"
)

file(
    GLOB_RECURSE _headers_list
    CONFIGURE_DEPENDS
    "${_src_root_path}/*.h" "${_src_root_path}/*.hpp"
)

find_package(Python COMPONENTS Interpreter REQUIRED)

execute_process(
    COMMAND ${Python_EXECUTABLE} "${_src_root_path}/preprocessing/generate_dll_interfaces.py" --headers "${_headers_list}" --common-lib "${CMAKE_SOURCE_DIR}/common" --output "\"${CMAKE_SOURCE_DIR}/api\"" --config "${_src_root_path}/preprocessing/config.json"
	WORKING_DIRECTORY "${_src_root_path}/preprocessing"
	COMMAND_ECHO STDOUT
)

file(STRINGS excluded_sources.txt excluded_sources_list)
foreach(elem ${excluded_sources_list})
list(REMOVE_ITEM _sources_list ${elem})
endforeach(elem)

source_group(TREE ${_src_root_path} PREFIX "Source files" FILES ${_sources_list})
source_group(TREE ${_src_root_path} PREFIX "Header files" FILES ${_headers_list})

add_library(${_target} SHARED ${_sources_list} ${_headers_list})

target_include_directories(${_target} PRIVATE "${CMAKE_SOURCE_DIR}"
                                              "${CMAKE_SOURCE_DIR}/api"
                                              "${CMAKE_SOURCE_DIR}/3rd_party/"
                                              "${CMAKE_SOURCE_DIR}/3rd_party/dxcompiler/inc/"
                                              "${CMAKE_BINARY_DIR}/packages/WinPixEventRuntime.1.0.200127001/Include/WinPixEventRuntime/")

target_link_directories(${_target} PRIVATE "${CMAKE_SOURCE_DIR}/3rd_party/dxcompiler/lib/x64")
target_link_directories(${_target} PRIVATE "${CMAKE_BINARY_DIR}/packages/WinPixEventRuntime.1.0.200127001/bin/x64/")
target_link_directories(${_target} PRIVATE "${CMAKE_BINARY_DIR}/bin/lib")
target_link_libraries(${_target} PRIVATE 
                      "glm_static.lib" "lua.lib" "imgui.lib" "zlib.lib" "pugixml.lib" "spookyhashv2.lib"
                      "d3dcompiler.lib" "dxgi.lib" "d3d12.lib" "dxcompiler.lib" "WinPixEventRuntime.lib" "common.lib")

add_dependencies(${_target} zlib glm lua imgui pugixml spookyhashv2 common)

if (MSVC)
    add_compile_definitions(_UNICODE)
    add_compile_definitions(UNICODE)

    find_program(NUGET nuget ${CMAKE_SOURCE_DIR})
    if(NOT NUGET)
        message(FATAL_ERROR "CMake was not able to locate nuget command line tool")
    endif()

    set_property(TARGET ${_target} PROPERTY VS_PACKAGE_REFERENCES "WinPixEventRuntime_1.0.200127001")

    # Configure Nuget packages for the project
    configure_file(packages.config.in packages.config COPYONLY)

    # Run Nuget to install any missing dependencies using packages.config
    execute_process(COMMAND ${NUGET} restore packages.config -SolutionDirectory ${CMAKE_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
		
	file(COPY ${CMAKE_BINARY_DIR}/packages/WinPixEventRuntime.1.0.200127001/bin/${CMAKE_VS_PLATFORM_NAME}/WinPixEventRuntime.dll DESTINATION ${_build_runtime_output_directory})
endif()

set_property(TARGET ${_target} PROPERTY OUTPUT_NAME "lexgine")