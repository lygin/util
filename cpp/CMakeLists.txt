cmake_minimum_required (VERSION 2.8)

project(base_compo_cpp)
set(CMAKE_CXX_STANDARD 14)


add_executable(rwlock rwlock_test.cc)
add_executable(cache cache_test.cc)
add_executable(easylog_test easylog.cc easylog_test.cc )
add_executable(doctest_demo doctest_demo.cc)
add_executable(threadpool_test threadpool_test.cc easylog.cc)

target_link_libraries(rwlock pthread)
target_link_libraries(easylog_test pthread)
target_link_libraries(threadpool_test pthread)