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
const int N = 512;
void done(io_context_t ctx, struct iocb *iocb, long res, long res2) {
  printf("done\n");
}
int main()
{
  int fd = open("testfile.txt", O_RDWR | O_CREAT | O_TRUNC | O_DIRECT, 0644);
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
  char * buf = NULL;
  posix_memalign((void**)&buf, N, N);
  memset(buf, 0, N);
  for(int i=0; i<N; i++) buf[i] = 'a';
  io_prep_pwrite(cb, fd, buf, N, 0);
  io_set_callback(cb,done);
  ret = io_submit(ctx, 1, &cb);
  printf("submit ret %d\n", ret);
  if (ret < 0)
  {
    perror("io_submit");
  }
  struct io_event events[2];
  while(true) {
    ret = io_getevents(ctx, 0, 2, events, NULL);
    if (ret < 0)
    {
      perror("io_getevents");
    }
    if(ret >= 1) break;
  }
  
  printf("ret: %d\n", ret);
  for (int i = 0; i < ret; i++)
  {
    struct iocb *completed_iocb = events[i].obj;
    // Process the completed I/O operation here
    io_callback_t func = (io_callback_t)completed_iocb->data;
    func(ctx, completed_iocb, 0, 0);
  }
  io_destroy(ctx);
  return 0;
}