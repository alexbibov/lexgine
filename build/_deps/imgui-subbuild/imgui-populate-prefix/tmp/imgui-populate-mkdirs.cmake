# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/ubuntu/.CPM/imgui/cf53")
  file(MAKE_DIRECTORY "/home/ubuntu/.CPM/imgui/cf53")
endif()
file(MAKE_DIRECTORY
  "/workspace/build/_deps/imgui-build"
  "/workspace/build/_deps/imgui-subbuild/imgui-populate-prefix"
  "/workspace/build/_deps/imgui-subbuild/imgui-populate-prefix/tmp"
  "/workspace/build/_deps/imgui-subbuild/imgui-populate-prefix/src/imgui-populate-stamp"
  "/workspace/build/_deps/imgui-subbuild/imgui-populate-prefix/src"
  "/workspace/build/_deps/imgui-subbuild/imgui-populate-prefix/src/imgui-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/workspace/build/_deps/imgui-subbuild/imgui-populate-prefix/src/imgui-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/workspace/build/_deps/imgui-subbuild/imgui-populate-prefix/src/imgui-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
