all: liburing_cat io_uring_cat

liburing_cat: liburing_cat.c
	gcc -o $@ $< -luring
io_uring_cat: io_uring_cat.c
	gcc -o $@ $< 