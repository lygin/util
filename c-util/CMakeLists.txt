cmake_minimum_required (VERSION 2.8)

project(base_compo_c)

add_executable(ae_timer_test ae_timer_test.c ae.c)
add_executable(slab_test slab_test.c slab.c)
add_executable(mempool mempool.c)

