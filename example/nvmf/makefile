
SPDK_LIB := $(shell pkg-config spdk_nvme --libs) 
DPDK_LIB := $(shell pkg-config spdk_env_dpdk --libs) 
SYS_LIB := $(shell pkg-config  spdk_syslibs --libs)
SPDK_INCLUDE := ../deps/spdk/include
 
tgt: 
	gcc nvmf_tgt.c -o tgt -pthread -Wl,--whole-archive $(SPDK_LIB) $(DPDK_LIB) -Wl,--no-whole-archive $(SYS_LIB) -I$(SPDK_INCLUDE)