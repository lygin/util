#ifndef _SHMEM_H_
#define _SHMEM_H_
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct {
    unsigned char *addr; //共享内存起始地址  
    size_t        size; //共享内存空间大小
} ngx_shm_t;


int ngx_shm_alloc(ngx_shm_t *shm)
{
    int fd = open("/dev/zero", O_RDWR);
    shm->addr = (u_char *)mmap(NULL, shm->size,PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm->addr == MAP_FAILED) {
        printf("%s\n", strerror(errno));
        return -1;
    }
    return 0;
}

void ngx_shm_free(ngx_shm_t *shm)
{
    munmap((void *) shm->addr, shm->size);
}

#endif