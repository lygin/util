cmake_minimum_required(VERSION 3.10)
project(async_file_logging CXX)

if(NOT TARGET spdlog)
    # Stand-alone build
    find_package(spdlog REQUIRED)
endif()

# ---------------------------------------------------------------------------------------
# using pre-compiled library
# ---------------------------------------------------------------------------------------
add_executable(async_file_logging async_file_logging.cc)
target_link_libraries(async_file_logging PRIVATE spdlog::spdlog)
