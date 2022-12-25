cmake_minimum_required(VERSION 3.14)
project(unit_test)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 14)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

enable_testing()


set(cutil ../c-util)
set(cpputil ../cpp-util)
include_directories(${cutil} ${cpputil})

aux_source_directory(. src)

add_executable(
  test_main
  ${src}
  ${cutil}/adlist.c
)

target_link_libraries(
  test_main
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_main)