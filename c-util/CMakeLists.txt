cmake_minimum_required (VERSION 2.8)

project(base_compo_c)

include_directories(../cpp-util)
add_executable(slab_test slab_test.c slab.c)
