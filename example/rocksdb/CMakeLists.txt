cmake_minimum_required(VERSION 3.16)
project(simple_example)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "-O3")

include_directories(../../cpp-util)
aux_source_directory(. SRC)
add_executable(simple_example ${SRC})
target_link_libraries(simple_example rocksdb leveldb pthread)