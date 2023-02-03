cmake_minimum_required(VERSION 3.16)
project(simpleKV)
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
add_executable(simplekv_client simplekv_client.c)
add_executable(simplekv_server simplekv_server.cc ${cutil}/ae.cc)
target_link_libraries(simplekv_server rocksdb pthread)