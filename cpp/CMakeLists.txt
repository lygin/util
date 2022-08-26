cmake_minimum_required (VERSION 2.8)

project(base_compo_cpp)

add_compile_options(-std=c++14 -Wall -g)


add_executable(rwlock rwlock_test.cc)
add_executable(cache cache_test.cc)
add_executable(easylog_test easylog.cc easylog_test.cc )
add_executable(doctest_demo doctest_demo.cc)
add_executable(threadpool_test threadpool_test.cc easylog.cc)
add_executable(spinlock_test spinlock_test.cc spinlock.cc easylog.cc tc_threadpool.cc)
add_executable(bloomfilter bloomfilter_test.cc)
add_executable(arena_test arena_test.cc arena.cc)
add_executable(hashtable hashtable.cc)

target_link_libraries(rwlock pthread)
target_link_libraries(easylog_test pthread)
target_link_libraries(threadpool_test pthread)
target_link_libraries(spinlock_test pthread)