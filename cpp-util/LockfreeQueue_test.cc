#include <cstdio>
#include <thread>
#include "LockfreeQueue.h"

int main() {
  MPMCQueue<int> q(10);
  auto t1 = std::thread([&] {
    int v;
    q.pop(v);
    printf("t1 %d\n", v);
  });
  auto t2 = std::thread([&] {
    int v;
    q.pop(v);
    printf("t2 %d\n", v);
  });

  q.push(1);
  q.push(2);
  t1.join();
  t2.join();
  
  return 0;
}