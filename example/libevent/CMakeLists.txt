project(libevent)

add_executable(client
  client.cc
)

add_executable(echo_server
  echo_server.cc
)

target_link_libraries(client
  event
)

target_link_libraries(echo_server
  event
)