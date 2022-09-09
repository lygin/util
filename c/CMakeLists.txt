cmake_minimum_required (VERSION 2.8)

project(base_compo_c)

add_executable(hashtable hashtable.c)

add_executable(rte_ring rte_ring.c rte_ring_test.c)
add_executable(ae_client ae_client.c)
add_executable(ae_server ae_server.c ae.c)
add_executable(ae_timer_test ae_timer_test.c ae.c)

target_link_libraries(rte_ring pthread)
