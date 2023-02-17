#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

const char *file = "file.txt";
const int mapsize = 8192;

int main(int argc, char *argv[])
{
  int fd = open(file, O_RDWR | O_CREAT, 0644);
  if (fd < 0) {
    printf("Error opening file");
    exit(1);
  }
  ftruncate(fd, mapsize);
  char * fileptr = (char *)mmap(NULL, mapsize, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
  char buf[1024];
  for(int i=0; i<mapsize; i+=1024) {
    memcpy(buf, fileptr+i, 1024);
    printf("read pos %d from file: %s\n", i, buf);
  }
  for(int i=0; i<mapsize; i+=1024) {
    strcpy(fileptr+i, "hello world");
  }
  if(fileptr == MAP_FAILED) {
    printf("mapping failed");
    exit(1);
  }
  munmap(fileptr, mapsize);
  close(fd);
  return 0;
}