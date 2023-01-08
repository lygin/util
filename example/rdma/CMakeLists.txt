project(rdma)

add_executable(rdma_cli
    rdma_cli.c
    rdma_common.c
)



add_executable(rdma_server
    rdma_server.c
    rdma_common.c
)

target_link_libraries(rdma_server
    ibverbs
    rdmacm
    pthread
)

target_link_libraries(rdma_cli
    ibverbs
    rdmacm
    pthread
)