/**
 * TODO: compare to libco
*/
#include <gtest/gtest.h>

extern "C" {
#include "coroutine.h"
}


struct args {
  int n;
};

void foo(schedule *sche, void *_arg) {
  args *arg = (args*)_arg;
  int start = arg->n;
  for(int i = 0; i < 5; i++) {
    printf("coroutine %d: %d\n", coroutine_running(sche), start + i);
    coroutine_yield(sche);
  }
  // return
  // coroutine will be removed from schedule
  // and status will be set as DEAD
}


TEST(ucontext_coroutine, basic) {
  schedule *sche = coroutine_open();
  args arg1 = {0};
  args arg2 = {100};
  int co1 = coroutine_new(sche, foo, &arg1);
  int co2 = coroutine_new(sche, foo, &arg2);
  printf("main start\n");
  while(coroutine_status(sche, co1) && coroutine_status(sche, co2)) {
    coroutine_resume(sche, co1);
    coroutine_resume(sche, co2);
  }
  printf("main end\n");
}