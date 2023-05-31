#include "rte_ring.h"
#include "rte_debug.h"
#include "rte_mempool.h"
int main(int argc, char **argv) {
  int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_panic("Cannot init EAL\n");
  struct rte_ring * que = rte_ring_create("q1", 1024, SOCKET_ID_ANY, RING_F_MP_RTS_ENQ | RING_F_MC_RTS_DEQ);
  if(que == NULL) {
    printf("err\n");
    exit(1);
  }
  struct rte_mempool *pool = rte_mempool_create("p1", 1024, 4, 0, 0, NULL, NULL, NULL, NULL, SOCKET_ID_ANY, 0);
  if(pool == NULL) {
    printf("pool err\n");
    exit(1);
  }
  
  uint32_t *buf = NULL;
  ret = rte_mempool_get(pool, (void**)&buf);
  if(ret != 0) {
    printf("get obj err\n");
  } else {
    printf("ptr %p\n", buf);
  }
  *buf = 7;
  ret = rte_ring_enqueue(que, buf);
  printf("ret : %d\n", ret);

  uint32_t *val = (uint32_t*)malloc(4);
  ret = rte_ring_dequeue(que, (void**)&val);
  printf("ret : %d\n", ret);
  if(ret == 0) {
    printf("val: %u\n", *(uint32_t *)val);
  }
  return 0;
}