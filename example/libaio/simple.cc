#include <libaio.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
const int N = 4096;
int main()
{
  int fd = open("testfile.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0)
  {
    perror("open");
  }
  io_context_t ctx;
  int ret = io_setup(1, &ctx);
  if (ret < 0)
  {
    perror("io_setup");
  }
  struct iocb *cb = (struct iocb *)calloc(1, sizeof(struct iocb));
  char * buf = (char*)malloc(N);
  memset(buf, 0, N);
  for(int i=0; i<N; i++) buf[i] = 'a';
  io_prep_pwrite(cb, fd, buf, N, 0);
  ret = io_submit(ctx, 1, &cb);
  if (ret < 0)
  {
    perror("io_submit");
  }
  struct io_event events[2];
  struct timespec tm;
  tm.tv_sec = 0;
  tm.tv_nsec = 0;
  ret = io_getevents(ctx, 0, 2, events, &tm);
  if (ret < 0)
  {
    perror("io_getevents");
  }
  printf("ret: %d\n", ret);
  for (int i = 0; i < ret; i++)
  {
    struct iocb *completed_iocb = events[i].obj;
    // Process the completed I/O operation here
  }
  io_destroy(ctx);
  return 0;
}