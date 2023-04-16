/**
 * @brief
 * 将一页内存分割成多个大小相等的内存块,
 * 不同的页分割出来的内存块大小可以不同.我们把页面叫做page,用m_page数组管理page页面;
 * 一页分割出来的内存块叫做slot,用m_slot数组管理slot块.这里的m_slot表示manage slot
 *
 * nginx的slab方法对于内存的管理分为两级,页面管理一级,即m_page数组,slot块管理一级,
 * 用于管理page页面分割的slot块.
 */
#include <sys/types.h>
#include "logging.h"

#ifndef _NGX_SLAB_H_INCLUDED_

typedef struct ngx_slab_page_s ngx_slab_page_t;
typedef unsigned long int uintptr_t;
typedef uintptr_t ngx_uint_t;
typedef long int ngx_int_t;
typedef unsigned char u_char;

#define ngx_align_ptr(p, a) \
    (u_char *)(((uintptr_t)(p) + ((uintptr_t)a - 1)) & ~((uintptr_t)a - 1))

struct ngx_slab_page_s
{
    // 使用bitmap来标记page页面中的slot块是否使用.并将位图存储在slab中.slab是32为整形变量,即对应32块slot.
    uintptr_t slab;
    // 9个ngx_slab_page_s通过next连接在一起
    // 如果页中的ojb<128 = 128 或者>128 ,则next直接指向对应的页slots[slot].next = page; 同时pages_m[]指向page->next = &slots[slot];
    ngx_slab_page_t *next; // 在分配较小obj的时候，next指向下一个page页

    // 由于指针是4的倍数,那么后两位一定为0,此时我们可以利用指针的后两位做标记,充分利用空间. 用低两位记录NGX_SLAB_PAGE等标记
    // 如果页中的obj<128,标记该页中存储的是小与128的obj page->prev = (uintptr_t) &slots[slot] | NGX_SLAB_SMALL
    // obj=128 page->prev = (uintptr_t) &slots[slot] | NGX_SLAB_EXACT;
    uintptr_t prev; // 上一个page页
};
/*
共享内存的其实地址开始处数据:ngx_slab_pool_t + 9 * sizeof(ngx_slab_page_t)(slots_m[]) + pages * sizeof(ngx_slab_page_t)(pages_m[]) +pages*ngx_pagesize(这是实际的数据部分，
每个ngx_pagesize都由前面的一个ngx_slab_page_t进行管理，并且每个ngx_pagesize最前端第一个obj存放的是一个或者多个int类型bitmap，用于管理每块分配出去的内存)

m_slot[0]:链接page页面,并且page页面划分的slot块大小为2^3
m_slot[1]:链接page页面,并且page页面划分的slot块大小为2^4
m_slot[2]:链接page页面,并且page页面划分的slot块大小为2^5
……………….
m_slot[8]:链接page页面,并且page页面划分的slot块大小为2k(2048)

m_page数组:数组中每个元素对应一个page页.
m_page[0]对应page[0]页面
m_page[1]对应page[1]页面
m_page[2]对应page[2]页面
………………………….
m_page[k]对应page[k]页面
另外可能有的m_page[]没有相应页面与他相对应.
*/
typedef struct
{                    // 初始化赋值在ngx_slab_init  slab结构是配合共享内存使用的  可以以limit req模块为例，参考ngx_http_limit_req_module
    size_t min_size; // 内存缓存obj最小的大小，一般是1个byte   //最小分配的空间是8byte 见ngx_slab_init
    // slab pool以shift来比较和计算所需分配的obj大小、每个缓存页能够容纳obj个数以及所分配的页在缓存空间的位置
    size_t min_shift; // ngx_init_zone_pool中默认为3
    // 指向ngx_slab_pool_t + 9 * sizeof(ngx_slab_page_t) + pages * sizeof(ngx_slab_page_t) +pages*ngx_pagesize(这是实际的数据部分)中的pages * sizeof(ngx_slab_page_t)开头处
    ngx_slab_page_t *pages; // 固定指向m_page数组.它的用途是用于定位管理,初始指向pages * sizeof(ngx_slab_page_t)首地址
    ngx_slab_page_t *last;  // 也就是指向实际的数据页pages*ngx_pagesize，指向最后一个pages页
    ngx_slab_page_t free;   // 是一个链表头,用于连接空闲页面.

    u_char *start; // 实际缓存obj的空间的开头   这个是对地址空间进行ngx_pagesize对齐后的起始地址，见ngx_slab_init
    u_char *end;

    u_char *log_ctx; // pool->log_ctx = &pool->zero;
    u_char zero;

    unsigned log_nomem : 1; // ngx_slab_init中默认为1

    // ngx_http_file_cache_init中cache->shpool->data = cache->sh;
    void *data; // 指向ngx_http_file_cache_t->sh
    void *addr; // 指向ngx_slab_pool_t的开头    //指向共享内存ngx_shm_zone_t中的addr+size尾部地址
} ngx_slab_pool_t;
void ngx_slab_init(ngx_slab_pool_t *pool);
void *ngx_slab_alloc_unlocked(ngx_slab_pool_t *pool, size_t size);
void *ngx_slab_calloc_unlocked(ngx_slab_pool_t *pool, size_t size);
void ngx_slab_free_unlocked(ngx_slab_pool_t *pool, void *p);

#endif /* _NGX_SLAB_H_INCLUDED_ */
