#define _GNU_SOURCE
#include <sched.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>


void worker(int bind_cpu) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(bind_cpu, &cpuset);
  /* 0表示绑定当前正在运行的线程 */
  sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
  while(1) {}
}

int main() {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(4, &cpuset);
  sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
  pthread_t t1,t2;
  pthread_create(&t1, NULL, (void*)worker, 5);
  pthread_create(&t1, NULL, (void*)worker, 6);
  while(1);
}