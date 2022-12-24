cmake_minimum_required (VERSION 2.8)

project(base_compo_c)

add_executable(hashtable hashtable.c)

add_executable(rte_ring rte_ring.c rte_ring_test.c)
add_executable(ae_timer_test ae_timer_test.c ae.c)
add_executable(ngxlock_test ngxlock.c ngxlock_test.c)
add_executable(slab_test slab_test.c slab.c)
add_executable(mempool mempool.c)

target_link_libraries(rte_ring pthread)
target_link_libraries(ngxlock_test pthread)

