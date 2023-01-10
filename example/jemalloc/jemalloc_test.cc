#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <jemalloc/jemalloc.h>
typedef unsigned long long ticks;
static __inline__ ticks getticks(void)
{
    uint32_t a, d;

    asm volatile("rdtsc" : "=a" (a), "=d" (d));
    return (((ticks)a) | (((ticks)d) << 32));
}
const int MALLOC_NUM = 50'0000;
int malloc_size[7] = {4, 8, 16, 32, 64, 128, 256};
void *ptrs[MALLOC_NUM];

int main() {
  ticks start = getticks();
  for(int i=0; i<MALLOC_NUM; i++) {
    ptrs[i] = malloc(malloc_size[i%7]);
  }
  for(int i=0; i<MALLOC_NUM; i++) {
    free(ptrs[i]);
  }
  ticks ed = getticks();
  printf("time %llu\n", ed - start);
  // Dump allocator statistics to stderr.
  // malloc_stats_print(NULL, NULL, NULL);
  return 0;
}

/**
 * 结果：
 * jemalloc avg: 81179584
 * glibc abg: 114696320
 * faster 41.28%
*/