cmake_minimum_required (VERSION 2.8)

project(base_compo_cpp)

add_compile_options(-std=c++14 -Wall -g -O3 -mavx512f)


include_directories(../c-util)
add_executable(page_cache_test  page_cache_test.cc disk_manager.cc)
add_executable(string_util_test string_util_test.cc)
add_executable(smart_ptr_test  smart_ptr_test.cc)
add_executable(rte_memcpy_test rte_memcpy_test.cc)
add_executable(cmdline_test cmdline_test.cc)

