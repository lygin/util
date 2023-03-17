Project(cache_test)
set(CMAKE_CXX_STANDARD 14)

include_directories(../)

aux_source_directory(. SRC)
add_executable(test ${SRC})
