cmake_minimum_required(VERSION 3.17)
set(_project_name api)
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

add_library(${_target} STATIC ${_sources_list} ${_headers_list})

target_include_directories(${_target} PUBLIC "${CMAKE_SOURCE_DIR}/" "${CMAKE_SOURCE_DIR}/api")
target_link_directories(${_target} PRIVATE "${CMAKE_BINARY_DIR}/bin/lib")
target_link_libraries(${_target} PRIVATE  "glm_static.lib" "zlib.lib" "pugixml.lib")

target_compile_definitions(${_target} PUBLIC LEXGINE_API_LIB)
add_dependencies(${_target} engine common)