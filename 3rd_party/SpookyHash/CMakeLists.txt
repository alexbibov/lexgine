cmake_minimum_required(VERSION 3.17)
set(_project_name spookyhashv2)
project(${_project_name})

set(_src_root_path ${PROJECT_SOURCE_DIR})
set(_tests_root_path ${PROJECT_SOURCE_DIR}/tests)
set(_target ${_project_name})

file(
    GLOB_RECURSE _sources_list
    "${_src_root_path}/*.c" "${_src_root_path}/*.cpp"
)

file(
    GLOB_RECURSE _headers_list
    "${_src_root_path}/*.h" "${_src_root_path}/*.hpp"
)

# Remove test sources
foreach(elem ${_sources_list})
cmake_path(IS_PREFIX _tests_root_path ${elem} result)
if(result)
list(REMOVE_ITEM _sources_list ${elem})
endif() 
endforeach(elem)

add_library(${_target} STATIC ${_sources_list})

source_group(TREE ${_src_root_path} PREFIX "Source files" FILES ${_sources_list})
source_group(TREE ${_src_root_path} PREFIX "Header files" FILES ${_headers_list})
                      
set_property(TARGET ${_target} PROPERTY OUTPUT_NAME "spookyhashv2")

enable_testing()
add_executable(spookyhashv2_tests tests/test_spooky_v2.cpp)
target_include_directories(spookyhashv2_tests PRIVATE "${CMAKE_SOURCE_DIR}")
target_link_libraries(spookyhashv2_tests GTest::gtest_main)
target_link_libraries(spookyhashv2_tests $<TARGET_OBJECTS:${_target}>)
add_dependencies(spookyhashv2_tests ${_target})

include(googletest)
gtest_discover_tests(spookyhashv2_tests)
