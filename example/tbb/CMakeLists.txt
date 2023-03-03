cmake_minimum_required(VERSION 3.16)
project(tbb_hashmap)
set(CMAKE_CXX_STANDARD 14)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE DEBUG)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(../../cpp-util)
aux_source_directory(. SRC)
add_executable(hashmap ${SRC})
target_link_libraries(hashmap tbb)