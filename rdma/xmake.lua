add_ldflags("-libverbs -lrdmacm -lpthread")

target("rdma-server")
    set_kind("binary")
    add_files("rdma_common.cc", "rdma_server.cc")
    add_includedirs("$(projectdir)/c_utils")