cmake_minimum_required(VERSION 3.14)
project(unit_test)

# GoogleTest requires at least C++14
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

enable_testing()


set(cutil ../c-util)
set(cpputil ../cpp-util)
include_directories(${cutil} ${cpputil} ${CLHT_DIR}/include)

aux_source_directory(. src)

add_executable(
  test_main
  ${src}
  ${cutil}/coroutine.c
  ${cpputil}/disk_manager.cc
  ${cpputil}/epoller.cc
)

target_link_libraries(
  test_main
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_main)