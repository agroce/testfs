# Copyright (c) 2017 Trail of Bits, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

project(testfs)
cmake_minimum_required(VERSION 2.8)

enable_language(C)
enable_language(CXX)

# Enable the GNU extensions
set(CMAKE_CXX_EXTENSIONS ON)

# Visual Studio already defaults to c++11
if (NOT WIN32)
  set(CMAKE_CXX_STANDARD 11)
endif ()

set(SOURCE_FILES
  Tests.cpp
  RamInterface.cpp
  posixtfs.c
  bitmap.c
  block.c
  csum.c
  dir.c
  file.c
  inode.c
  ops.c
  super.c
  tx.c)

add_executable(Tests
  ${SOURCE_FILES})

add_executable(Tests32
  ${SOURCE_FILES})

set_target_properties(Tests32
  PROPERTIES COMPILE_FLAGS "-m32 -g3" LINK_FLAGS "-m32 -g3")

set_target_properties(Tests
  PROPERTIES COMPILE_FLAGS "-g" LINK_FLAGS "-g")

target_link_libraries(Tests
  deepstate)

target_link_libraries(Tests32
  deepstate32)
