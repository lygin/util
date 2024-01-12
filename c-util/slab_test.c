#include "shmem.h"
#include "slab.h"

int main() {
  ngx_shm_t shm;
  shm.size = 1 << 20; // 1M
  if (ngx_shm_alloc(&shm) == -1) {
    LOG_INFO("SHM ERROR");
    return 0;
  }
  ngx_slab_pool_t *pool;
  pool = (ngx_slab_pool_t*)shm.addr;
  pool->end = shm.addr + shm.size;
  pool->min_shift = 3;
  pool->addr = shm.addr;

  ngx_slab_init(pool);
  int *p = ngx_slab_alloc(pool, sizeof(int));
  *p = 9;
  LOG_INFO("%d", *p);
  ngx_slab_free(pool, p);
  return 0;
}