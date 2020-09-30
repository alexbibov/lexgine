cmake_minimum_required(VERSION 3.17)
set(_project_name pugixml)
project(${_project_name})

set(_src_root_path ${PROJECT_SOURCE_DIR})
set(_target ${_project_name})

file(GLOB _sources_list
     LIST_DIRECTORIES false
     "${_src_root_path}/*.c" "${_src_root_path}/*.cpp" 
)

file(GLOB _headers_list
     LIST_DIRECTORIES false
     "${_src_root_path}/*.h" "${_src_root_path}/*.hpp" 
)

add_library(${_target} STATIC ${_sources_list})

source_group("Source files" FILES ${_sources_list})
source_group("Heder files" FILES ${_headers_list})

set_property(TARGET ${_target} PROPERTY OUTPUT_NAME "pugixml")