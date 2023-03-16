cmake_minimum_required(VERSION 3.16)
project(simpleDB)
set(CMAKE_CXX_STANDARD 14)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE DEBUG)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(apmemobj libpmemobj.cc)
target_link_libraries(apmemobj pmemobj)

add_executable(bpmem libpmem.cc)
target_link_libraries(bpmem pmem)