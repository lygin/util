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
    // 在分配较小obj的时候，next指向下一个page页
    ngx_slab_page_t *next;
    // 由于指针是4的倍数,那么后两位一定为0,此时我们可以利用指针的后两位做标记,充分利用空间. 用低两位记录NGX_SLAB_PAGE等标记
    uintptr_t prev; // 上一个page页
};
/*
m_slot[0]:链接page页面,并且page页面划分的slot块大小为8
m_slot[1]:链接page页面,并且page页面划分的slot块大小为16
m_slot[2]:链接page页面,并且page页面划分的slot块大小为32
m_slot[8]:链接page页面,并且page页面划分的slot块大小为2048

m_page数组:数组中每个元素对应一个page页.
m_page[0]对应page[0]页面
m_page[1]对应page[1]页面
m_page[2]对应page[2]页面
*/
typedef struct
{
    size_t min_size; // 内存缓存obj最小的大小 8B
    size_t min_shift; // ngx_init_zone_pool中默认为3

    ngx_slab_page_t *pages; // 固定指向m_page数组.它的用途是用于定位管理,初始指向pages * sizeof(ngx_slab_page_t)首地址
    ngx_slab_page_t *last;  // 也就是指向实际的数据页pages*ngx_pagesize，指向最后一个pages页
    ngx_slab_page_t free;   // 是一个链表头,用于连接空闲页面.

    u_char *start; // 实际缓存obj的空间的开头
    u_char *end;

    u_char *log_ctx; // pool->log_ctx = &pool->zero;
    u_char zero;

    unsigned log_nomem : 1; // ngx_slab_init中默认为1

    void *data; // 指向ngx_http_file_cache_t->sh
    void *addr; // 指向ngx_slab_pool_t的开头
} ngx_slab_pool_t;

void ngx_slab_init(ngx_slab_pool_t *pool);
void *ngx_slab_alloc(ngx_slab_pool_t *pool, size_t size);
void *ngx_slab_calloc(ngx_slab_pool_t *pool, size_t size);
void ngx_slab_free(ngx_slab_pool_t *pool, void *p);

#endif /* _NGX_SLAB_H_INCLUDED_ */
