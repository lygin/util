#ifndef _SHMEM_H_
#define _SHMEM_H_
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>
/*
 * 共享内存的其实地址开始处数据:ngx_slab_pool_t + 9 * sizeof(ngx_slab_page_t)(slots_m[]) + pages * sizeof(ngx_slab_page_t)(pages_m[]) +pages*ngx_pagesize(这是实际的数据部分，
 * 每个ngx_pagesize都由前面的一个ngx_slab_page_t进行管理，并且每个ngx_pagesize最前端第一个obj存放的是一个或者多个int类型bitmap，用于管理每块分配出去的内存)
 * 见ngx_init_zone_pool，共享内存的起始地址开始的sizeof(ngx_slab_pool_t)字节是用来存储管理共享内存的slab poll的
 */
typedef struct {
    unsigned char      *addr; //共享内存起始地址  
    size_t       size; //共享内存空间大小
} ngx_shm_t;


int ngx_shm_alloc(ngx_shm_t *shm)
{
    shm->addr = (u_char *) mmap(NULL, shm->size,PROT_READ|PROT_WRITE, MAP_SHARED, -1, 0);
    if (shm->addr == MAP_FAILED) {
        return -1;
    }

    return 0;
}

void ngx_shm_free(ngx_shm_t *shm)
{
    munmap((void *) shm->addr, shm->size);
}

#endif