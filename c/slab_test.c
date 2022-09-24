#include "shmem.h"
#include "slab.h"

int main() {
  ngx_shm_t shm;
  shm.size = 1 << 20; // 4M
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
  LOG_INFO("%d", sizeof(int));
  int *p = ngx_slab_alloc_unlocked(pool, sizeof(int));
  *p = 9;
  LOG_INFO("%d", *p);
  ngx_slab_free_unlocked(pool, p);
  return 0;
}