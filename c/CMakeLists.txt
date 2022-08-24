cmake_minimum_required (VERSION 2.8)

project(base_compo_c)

add_executable(hash hash.c)

add_executable(rte_ring rte_ring.c rte_ring_test.c)

target_link_libraries(rte_ring pthread)
