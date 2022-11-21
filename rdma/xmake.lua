add_ldflags("-libverbs -lrdmacm -lpthread")

target("rdma-server")
    set_kind("binary")
    add_files("rdma_common.c", "rdma_server.c")
    
target("rdma-client")
    set_kind("binary")
    add_files("rdma_common.c", "rdma_cli.c")