cmake_minimum_required(VERSION 3.17)
set(_project_name common)
project(${_project_name})

set(_src_root_path ${PROJECT_SOURCE_DIR})
set(_target ${_project_name})

file(STRINGS "${PROJECT_BINARY_DIR}/headers_list.txt" _headers_list)
file(STRINGS "${PROJECT_BINARY_DIR}/sources_list.txt" _sources_list)

list(APPEND _headers_list "${PROJECT_BINARY_DIR}/ioc_traits.h")

#source_group(TREE ${_src_root_path} "Header files" FILES ${_headers_list})
#source_group(TREE ${_src_root_path} "Source files" FILES ${_sources_list})

add_library(${_target} STATIC ${_sources_list} ${_headers_list})

target_include_directories(${_target} PRIVATE "${CMAKE_SOURCE_DIR}/")
target_link_directories(${_target} PRIVATE "${CMAKE_BINARY_DIR}/bin/lib")
target_link_libraries(${_target} PRIVATE  "glm_static.lib" "zlib.lib" "pugixml.lib")


add_dependencies(${_target} zlib glm)
set_property(TARGET ${_target} PROPERTY OUTPUT_NAME "common")