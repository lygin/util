/*
libaio without O_direct = sync io (no use)
libaio with O_direct attention:
1.buffer alignment and size must be multiple of 512B
2.file offset must be multiple of 512B
*/
#include <libaio.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "aioworker.h"
#include <future>
#include "timer.h"
#include <unistd.h>
const int N = 1024;
const int BATCH_SIZE = 4096;

int main()
{
  int fd = open("testfile.txt", O_RDWR | O_CREAT | O_TRUNC | O_DIRECT, 0644);
  if (fd < 0)
  {
    perror("open");
  }
  AIOworker2 store;
  char * buf[BATCH_SIZE] = {0};
  for (int i=0; i<BATCH_SIZE; i++) {
    posix_memalign((void**)&buf[i], 512, N);
    memset(buf[i], 0, N);
    for(int j=0; j<N; j++) buf[i][j] = 'a'+i;
  } 
  WaitGroup wg;
  Task t[BATCH_SIZE];
  wg.Add(BATCH_SIZE);
  Timer tm;
  for (int i=0; i<BATCH_SIZE; i++) {
      t[i].op = WRITE;
      t[i].fd = fd;
      t[i].buf = buf[i];
      t[i].off = i*N;
      t[i].len = N;
      t[i].wg = &wg;
      store.submit_task(&t[i]);
  }
  wg.Wait();
  double elapsed = tm.GetDurationMs();
  LOG_INFO("insert IOPS: %.2fKops\n avg latency: %.2fus", BATCH_SIZE*1.0/elapsed, elapsed*1024/BATCH_SIZE);

  char bufr[N] = {0};
  int ret = read(fd, buf[0], N);
  printf("read: %d %s\n", ret, buf[0]);
  
  
  return 0;
}