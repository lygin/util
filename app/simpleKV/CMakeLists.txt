project(simpleKV)

set(util_dir "/home/lj/util/c-util")
add_definitions(
  -Wall
  -g
  -Og
)



include_directories(${util_dir})
add_executable(simplekv_client simplekv_client.c)
add_executable(simplekv_server ${util_dir}/ae.c ${util_dir}/hashtable.c simplekv_server.c)