cmake_minimum_required(VERSION 3.23)
set(_project_name engine)
project(${_project_name})

set(_src_root_path ${PROJECT_SOURCE_DIR})
set(_tests_root_path ${PROJECT_SOURCE_DIR}/tests)
set(_target ${_project_name})

function (configure_openmp_support target_name)
	if(OpenMP_CXX_FOUND)
	target_compile_options(${target_name} PRIVATE ${OpenMP_CXX_FLAGS})
	target_include_directories(${target_name} PRIVATE ${OpenMP_CXX_INCLUDE_DIRS})
	target_link_libraries(${target_name} PRIVATE ${OpenMP_CXX_LIBRARIES})
	endif()
endfunction()

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
find_package(OpenMP)

execute_process(
    COMMAND ${Python_EXECUTABLE} "${_src_root_path}/preprocessing/generate_dll_interfaces.py" --source-tree "${CMAKE_SOURCE_DIR}" --headers "${_headers_list}" --common-lib "${CMAKE_BINARY_DIR}/common" --output "${CMAKE_BINARY_DIR}" --config "${_src_root_path}/preprocessing/config.json"
    WORKING_DIRECTORY "${_src_root_path}/preprocessing"
    COMMAND_ECHO STDOUT
)

file(
    GLOB_RECURSE _generated_sources_list
    CONFIGURE_DEPENDS
    "${PROJECT_BINARY_DIR}/*.c" "${PROJECT_BINARY_DIR}/*.cpp"
)

list(APPEND ${_headers_list} "${CMAKE_PROJECT_BINARY_DIR}/build_info.h")

# Remove test sources
foreach(elem ${_sources_list})
cmake_path(IS_PREFIX _tests_root_path ${elem} result)
if(result)
list(REMOVE_ITEM _sources_list ${elem})
endif() 
endforeach(elem)

# Remove excluded sources
file(STRINGS ${PROJECT_BINARY_DIR}/excluded_sources.txt excluded_sources_list)
foreach(elem ${excluded_sources_list})
list(REMOVE_ITEM _sources_list ${elem})
endforeach(elem)

source_group(TREE ${_src_root_path} PREFIX "Source files" FILES ${_sources_list})
source_group(TREE ${_src_root_path} PREFIX "Header files" FILES ${_headers_list})
source_group(TREE ${PROJECT_BINARY_DIR} PREFIX "Generated sources" FILES ${_generated_sources_list})

add_library(${_target} SHARED ${_sources_list} ${_generated_sources_list} ${_headers_list})

target_include_directories(${_target} PRIVATE "${CMAKE_SOURCE_DIR}"
                                              "${CMAKE_SOURCE_DIR}/api"
                                              "${CMAKE_SOURCE_DIR}/3rd_party/"
                                              "${CMAKE_SOURCE_DIR}/3rd_party/dxcompiler/inc/"
                                              "${CMAKE_BINARY_DIR}/packages/WinPixEventRuntime.1.0.240308001/Include/WinPixEventRuntime/"
                                              "${CMAKE_BINARY_DIR}"
                                              "${CMAKE_SOURCE_DIR}/3rd_party/DirectXTex/")

target_link_directories(${_target} PRIVATE "${CMAKE_SOURCE_DIR}/3rd_party/dxcompiler/lib/${CMAKE_VS_PLATFORM_NAME}")
target_link_directories(${_target} PRIVATE "${CMAKE_BINARY_DIR}/packages/WinPixEventRuntime.1.0.240308001/bin/${CMAKE_VS_PLATFORM_NAME}/")
target_link_directories(${_target} PRIVATE "${CMAKE_BINARY_DIR}/bin/lib")

set(_engine_lib_dependencies 
"glm_static.lib"
"imgui.lib" 
"zlib.lib" 
"pugixml.lib" 
"spookyhashv2.lib"
"ktx_read.lib"
"d3dcompiler.lib" 
"dxgi.lib" 
"d3d12.lib" 
"d3d11.lib"
"runtimeobject.lib"
"dxcompiler.lib" 
"WinPixEventRuntime.lib" 
"common.lib"
"DirectXTex.lib"
"BCrypt.lib"
)
target_link_libraries(${_target} PRIVATE ${_engine_lib_dependencies})

add_dependencies(${_target} zlib glm imgui pugixml spookyhashv2 ktx_read common DirectXTex)
target_compile_definitions(${_target} PUBLIC LEXGINE_GLOBAL_LOOKUP_PREFIX="${CMAKE_SOURCE_DIR}")
target_compile_definitions(${_target} PUBLIC LEXGINE_SETTINGS_PATH="settings/")
target_compile_definitions(${_target} PUBLIC LEXGINE_SCRIPTS_PATH="scripts/")
target_compile_definitions(${_target} PRIVATE LEXGINE_SHADERS_PATH="shaders/")
target_compile_definitions(${_target} PUBLIC USE_PIX)

configure_openmp_support(${_target})

if (MSVC)
    add_compile_definitions(_UNICODE)
    add_compile_definitions(UNICODE)

    find_program(NUGET nuget ${CMAKE_SOURCE_DIR})
    if(NOT NUGET)
        message(FATAL_ERROR "CMake was not able to locate nuget command line tool")
    endif()

    set_property(TARGET ${_target} PROPERTY VS_PACKAGE_REFERENCES "WinPixEventRuntime_1.0.240308001")

    # Configure Nuget packages for the project
    configure_file(packages.config.in packages.config COPYONLY)

    # Run Nuget to install any missing dependencies using packages.config
    execute_process(COMMAND ${NUGET} restore packages.config -SolutionDirectory ${CMAKE_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
		
	file(COPY ${CMAKE_BINARY_DIR}/packages/WinPixEventRuntime.1.0.240308001/bin/${CMAKE_VS_PLATFORM_NAME}/WinPixEventRuntime.dll DESTINATION ${_build_runtime_output_directory})
endif()

set(_dxc_binary_path ${CMAKE_SOURCE_DIR}/3rd_party/dxcompiler/bin/${CMAKE_VS_PLATFORM_NAME}/)
file(COPY ${_dxc_binary_path}/dxcompiler.dll DESTINATION ${_build_runtime_output_directory})
file(COPY ${_dxc_binary_path}/dxil.dll DESTINATION ${_build_runtime_output_directory})

set_property(TARGET ${_target} PROPERTY OUTPUT_NAME "lexgine")

add_executable(basic_tests tests/basic_tests.cpp)

target_include_directories(basic_tests PRIVATE "${CMAKE_SOURCE_DIR}"
                                               "${CMAKE_SOURCE_DIR}/api"
                                               "${CMAKE_SOURCE_DIR}/3rd_party/"
                                               "${CMAKE_SOURCE_DIR}/3rd_party/dxcompiler/inc/"
                                               "${CMAKE_BINARY_DIR}"
                                               "${CMAKE_SOURCE_DIR}/3rd_party/DirectXTex/")

target_link_directories(basic_tests PRIVATE "${CMAKE_SOURCE_DIR}/3rd_party/dxcompiler/lib/${CMAKE_VS_PLATFORM_NAME}")
target_link_directories(basic_tests PRIVATE "${CMAKE_BINARY_DIR}/packages/WinPixEventRuntime.1.0.240308001/bin/${CMAKE_VS_PLATFORM_NAME}/")
target_link_directories(basic_tests PRIVATE "${CMAKE_BINARY_DIR}/bin/lib")

target_link_libraries(basic_tests PRIVATE GTest::gtest_main)
target_link_libraries(basic_tests PRIVATE ${_engine_lib_dependencies})
target_link_libraries(basic_tests PRIVATE $<TARGET_OBJECTS:${_target}>)
target_link_libraries(basic_tests PRIVATE ${_target})

configure_openmp_support(basic_tests)

add_dependencies(basic_tests ${_target})

enable_testing()
include(googletest)
gtest_discover_tests(basic_tests TEST_FILTER CacheTest.*:EngineTests_Basic.*:EngineTests_Concurrency.*)