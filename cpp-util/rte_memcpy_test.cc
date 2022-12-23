#include "rte_memcpy.h"
#include "timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <random>

const int N = 1000000;

int main() {
  uint8_t a[64];
  uint8_t b[64];
  uint64_t t1 = getticks();
  for (int i = 0; i < N; ++i) {
    sprintf((char*)a, "%d", i);
    rte_memcpy_generic(b, a, 64);
    //printf("%s", (char*)b);
  }
  uint64_t t2 = getticks();
  printf("%llu\n", t2 - t1);
  t1 = getticks();
  for (int i = 0; i < N; ++i) {
    sprintf((char*)a, "%d", i);
    memcpy(b, a, 64);
    //printf("%s", (char*)b);
  }
  t2 = getticks();
  printf("%llu\n", t2 - t1);
}