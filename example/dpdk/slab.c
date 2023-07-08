#include "rte_debug.h"
#include "rte_mempool.h"
#define LEVEL 5
struct rte_mempool *pools[LEVEL];
static int elem[LEVEL] = { 8, 16, 32, 64, 128};
static int elem_total[LEVEL] = { 1024, 1024, 1024, 1024, 1024};


void init_allocator()
{
  for (int i=0; i<LEVEL; i++)
  {
    char buf[16] = {0};
    sprintf(buf, "a%d", elem[i]);
    pools[i] = rte_mempool_create(buf, elem_total[i], elem[i], 0, 0, NULL, NULL, NULL, NULL, SOCKET_ID_ANY, 0);
    if (pools[i] == NULL) {
      printf("init fail\n");
      abort();
    }
  }
}

void* slab_alloc(int size)
{
  int i = 0;
  if (size > elem[LEVEL-1]) return NULL;
  for(; i<LEVEL; ++i)
  {
    if (size <= elem[i]) break;
  }
  void *ret = NULL;
  int s = rte_mempool_get(pools[i], &ret);
  if (s != 0) {
    printf("alloc err\n");
    abort();
  }
  return ret;
}

int main(int argc, char **argv) {
  int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_panic("Cannot init EAL\n");
  init_allocator();
  printf("%p\n", slab_alloc(6));
  printf("%p\n", slab_alloc(7));
  printf("%p\n", slab_alloc(16));
  printf("%p\n", slab_alloc(26));
  printf("%p\n", slab_alloc(36));
  printf("%p\n", slab_alloc(66));
  // uint32_t *buf = NULL;
  // ret = rte_mempool_get(pool, (void**)&buf);
  // if(ret != 0) {
  //   printf("get obj err\n");
  // } else {
  //   printf("ptr %p\n", buf);
  // }
  return 0;
}