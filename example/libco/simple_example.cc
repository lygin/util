#include "co_routine.h"
#include <cstdio>
int cnt = 5;
static const int CONUM = 5;
void *hello(void *arg) {
  int *start = (int *)arg;
  for(int i = 0; i<cnt; ++i) {
    printf("%d\n", i+(*start));
    co_yield_ct();
  }
  return 0;
}

int main() {
  stCoRoutine_t *co[CONUM];
  
  for(int i=0; i<CONUM; ++i) {
    int *start = new int(100*i);
    co_create(&co[i], NULL, hello, start);
  }
  for(int i=0; i<cnt; ++i) {
    for(int i=0; i<CONUM; ++i) {
      co_resume(co[i]);
    }
  }
  return 0;
}