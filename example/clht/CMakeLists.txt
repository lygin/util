cmake_minimum_required(VERSION 3.16)
project(tbb_hashmap)
set(CMAKE_CXX_STANDARD 14)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE DEBUG)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CLHT_DIR ../../third_party/CLHT)
include_directories(${CLHT_DIR}/include)
link_directories(${CLHT_DIR} ${CLHT_DIR}/external/lib)

aux_source_directory(. SRC)
add_executable(test ${SRC})

target_link_libraries(test clht ssmem)

