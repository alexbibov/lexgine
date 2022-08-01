cmake_minimum_required(VERSION 3.17)
set(_project_name swe_demo)
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


source_group(TREE ${_src_root_path} PREFIX "Source files" FILES ${_sources_list})
source_group(TREE ${_src_root_path} PREFIX "Header files" FILES ${_headers_list})

add_executable(${_target} ${_sources_list} ${_headers_list})

add_dependencies(${_target} api)

target_include_directories(${_target} PRIVATE "${CMAKE_SOURCE_DIR}")
target_link_directories(${_target} PRIVATE "${CMAKE_BINARY_DIR}/bin/lib/")
target_link_libraries(${_target} PRIVATE "api.lib" "common.lib")
target_compile_definitions(${_target} PRIVATE LEXGINE_DLL_PATH=L"${CMAKE_BINARY_DIR}/bin/lexgine.dll" LEXGINE_API_LIB)

set_property(TARGET ${_target} PROPERTY OUTPUT_NAME "swe")