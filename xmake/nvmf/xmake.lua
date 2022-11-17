-- target("nvmf-tgt")
--     set_kind("binary")
--     add_files("nvmf/nvmf_tgt.c")
--     add_includedirs("deps/spdk/include")
    

-- target("nvmf-initiator")
--     set_kind("binary")
--     add_files("nvmf/nvmf_initiator.c")
--     add_includedirs("deps/spdk/include")
--     add_linkdirs("deps/spdk/build/lib")
--     add_links("spdk_accel", "spdk_log", "spdk_sock","spdk_util", "spdk_nvme", "spdk_thread", "spdk_json",
--         "spdk_jsonrpc", "spdk_rpc", "spdk_trace", "spdk_bdev", "spdk_rdma")