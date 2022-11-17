target("nvmf-tgt")
    set_kind("binary")
    add_files("nvmf_tgt.c")
    add_includedirs("$(projectdir)/deps/spdk/include")
    add_linkdirs("$(projectdir)/deps/spdk/build/lib")
    add_links("spdk_trace", "spdk_trace_parser", "spdk_thread")
    add_ldflags("-pthread",
    "-Wl,--whole-archive",
    "$(shell pkg-config --libs spdk_nvme)",
    "$(shell pkg-config --libs spdk_env_dpdk)",
    "-Wl,--no-whole-archive",
    "$(shell pkg-config --libs spdk_syslibs)")
    
target_end()

-- target("nvmf-initiator")
--     set_kind("binary")
--     add_files("nvmf/nvmf_initiator.c")
--     add_includedirs("deps/spdk/include")
--     add_linkdirs("deps/spdk/build/lib")
    