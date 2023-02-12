cmake_minimum_required (VERSION 2.8)

project(base_compo_cpp)

add_compile_options(-std=c++14 -Wall -g -O3 -mavx512f)


include_directories(../c-util)
add_executable(smart_ptr_test  smart_ptr_test.cc)
add_executable(cmdline_test cmdline_test.cc)

