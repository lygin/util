PKGCONF = pkg-config
APP = slab
SRCS = slab.c
CFLAGS += -O3 $(shell $(PKGCONF) --cflags libdpdk)
LDFLAGS += $(shell $(PKGCONF) --libs libdpdk)

$(APP): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)

clean: ring_test
	rm -rf $<