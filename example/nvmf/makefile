.PHONY: clean

CC = gcc
SPDK_ROOT_DIR = /home/lj/spdk
PKG_CONFIG_PATH = $(SPDK_ROOT_DIR)/build/lib/pkgconfig
LIBS += $(shell PKG_CONFIG_PATH="$(PKG_CONFIG_PATH)" pkg-config --libs spdk_nvme) 
LIBS += $(shell PKG_CONFIG_PATH="$(PKG_CONFIG_PATH)" pkg-config --libs spdk_env_dpdk)

LIBS += -lpthread -luuid -lrt


LD_FLAGS += -L$(SPDK_ROOT_DIR)/build/lib \
						-L$(SPDK_ROOT_DIR)/dpdk/build/lib

CFLAGS += -I$(SPDK_ROOT_DIR)/include

SRCS = nvmf_cli.c
 
cli: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $@ -Wl,--no-as-needed  -Wl,-rpath-link=$(SPDK_ROOT_DIR)/dpdk/build/lib \
		$(LD_FLAGS) $(LIBS)

clean:
	rm -f cli