cmake_minimum_required (VERSION 2.8)

project(base_compo_cpp)

add_compile_options(-std=c++14 -Wall -g -O3 -mavx512f)



add_executable(rwlock rwlock_test.cc)
add_executable(lrucache lrucache_test.cc)
add_executable(doctest_demo doctest_demo.cc)
add_executable(threadpool_test threadpool_test.cc)
add_executable(bloomfilter bloomfilter_test.cc)
add_executable(arena_test arena_test.cc arena.cc)
add_executable(hashtable_test hashtable_test.cc hashtable.cc)
add_executable(disk_manager_test disk_manager_test.cc disk_manager.cc)
add_executable(bufferpool_test  bufferpool_test.cc bufferpool.cc disk_manager.cc arena.cc)
add_executable(string_util_test string_util_test.cc)
add_executable(ringbuffer_test ringbuffer_test.cc ringbuffer.cc)
add_executable(smart_ptr_test  smart_ptr_test.cc)
add_executable(rte_memcpy_test rte_memcpy_test.cc)
add_executable(cmdline_test cmdline_test.cc)
add_executable(LockfreeQueue_test LockfreeQueue_test.cc)

#find_package(mimalloc 2.0 REQUIRED)
#target_link_libraries(arena_test mimalloc)
target_link_libraries(rwlock pthread)
target_link_libraries(threadpool_test pthread)
target_link_libraries(ringbuffer_test pthread)
target_link_libraries(LockfreeQueue_test pthread)
