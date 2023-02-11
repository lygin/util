cmake_minimum_required(VERSION 3.16)
project(simpleDB)
set(CMAKE_CXX_STANDARD 14)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE DEBUG)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(cutil ../../c-util)
set(cpputil ../../cpp-util)


include_directories(${cutil} ${cpputil})
aux_source_directory(. SRC)
add_executable(simpledb_test ${SRC} ${cpputil}/disk_manager.cc)
target_link_libraries(simpledb_test pthread)