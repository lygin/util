cmake_minimum_required(VERSION 3.16)
project(mmap)

aux_source_directory(. SRC)
add_executable(main ${SRC})