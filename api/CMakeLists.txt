cmake_minimum_required(VERSION 3.17)
set(_project_name api)
project(${_project_name})

set(_src_root_path ${PROJECT_BINARY_DIR})
set(_target ${_project_name})

file(
    GLOB_RECURSE _generated_sources_list
    CONFIGURE_DEPENDS
    "${_src_root_path}/*.c" "${_src_root_path}/*.cpp"
)
file(
    GLOB_RECURSE _generated_headers_list
    CONFIGURE_DEPENDS
    "${_src_root_path}/*.h" "${_src_root_path}/*.hpp"
)

list(APPEND _sources_list 
"${PROJECT_SOURCE_DIR}/ioc.cpp" 
"${PROJECT_SOURCE_DIR}/lexgine_object.cpp" 
"${PROJECT_SOURCE_DIR}/link_result.cpp"
)

list(APPEND _headers_list 
"${PROJECT_SOURCE_DIR}/ioc.h" 
"${PROJECT_SOURCE_DIR}/is_call_possible.h" 
"${PROJECT_SOURCE_DIR}/lexgine_object.h"
"${PROJECT_SOURCE_DIR}/link_result.h"
)

source_group(TREE ${_src_root_path} PREFIX "Generated Source files" FILES ${_generated_sources_list})
source_group(TREE ${_src_root_path} PREFIX "Generated Header files" FILES ${_generated_headers_list})
source_group(TREE ${PROJECT_SOURCE_DIR} PREFIX "Source files" FILES ${_sources_list})
source_group(TREE ${PROJECT_SOURCE_DIR} PREFIX "Header files" FILES ${_headers_list})

add_library(${_target} STATIC ${_generated_sources_list} ${_sources_list} ${_generated_headers_list} ${_headers_list})

target_include_directories(${_target} PUBLIC "${CMAKE_BINARY_DIR}" "${PROJECT_BINARY_DIR}" "${CMAKE_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}")
target_link_directories(${_target} PUBLIC "${CMAKE_BINARY_DIR}/bin/lib")
target_link_libraries(${_target} PUBLIC common)

target_compile_definitions(${_target} PUBLIC LEXGINE_API_LIB)
add_dependencies(${_target} engine common)