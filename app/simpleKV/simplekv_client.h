#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <string>
#include "logging.h"
#include "lockfreeQueue.h"

ssize_t readline(int fd, char *vptr, size_t maxlen)
{
	ssize_t n, rc;
	char c, *ptr;

	ptr = vptr;
	for (n = 1; n < maxlen; n++)
	{
		if ((rc = read(fd, &c, 1)) == 1)
		{
			*ptr++ = c;
			if (c == '\n')
				break; /* newline is stored, like fgets() */
		}
		else if (rc == 0)
		{
			*ptr = 0;
			return n - 1; /* EOF, n - 1 bytes were read */
		}
		else
			return -1; /* error, errno set by read() */
	}

	*ptr = 0; /* null terminate like fgets() */
	return n;
}

class SimpleKVClient {
  static const int kPort = 7778;
  static const int kMaxMsg = 2048;
  int *fds_;
  MPMCQueue<int> free_fds_;

public:
  SimpleKVClient(const std::string& server, int fds): fds_(new int[fds]),  free_fds_(fds<<1){
    for(int i=0; i<fds; ++i) {
      if(init(server, fds_[i]) != 0) {
        LOG_ERROR("init fd fail");
        break;
      }
      free_fds_.push(fds_[i]);
    }
  }
  int init(const std::string& ip, int &fd) {
    struct sockaddr_in servaddr;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
      LOG_ERROR("socket error");
      return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(kPort);
    if (inet_pton(AF_INET, ip.c_str(), &servaddr.sin_addr) < 0)
    {
      printf("inet_pton error for %s\n", ip.c_str());
      return -1;
    }

    if (connect(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
      LOG_ERROR("connect error");
      return -1;
    }
    LOG_INFO("fd %d connected\n", fd);
    return 0;
  }

  int put(const std::string& key, const std::string& value) {
    char sendbuf[kMaxMsg] = {0}, recvbuf[kMaxMsg];
    int bufsize = sprintf(sendbuf, "put %s %s", key.c_str(), value.c_str());
    LOG_INFO("req %s", sendbuf);
    int fd;
    LOG_INFO("free_fds_ size: %d", free_fds_.size());
    free_fds_.pop(fd);
    LOG_INFO("pop fd %d\n", fd);
    int n = write(fd, sendbuf, bufsize);
    LOG_INFO("write len %d\n", n);
    if(n <= 0) {
      perror("write request error");
      free_fds_.push(fd);
      return -1;
    }
    n = readline(fd, recvbuf, kMaxMsg);
    LOG_INFO("recv: %s", recvbuf);
    free_fds_.push(fd);
    if(n <= 0) {
      perror("read response fail");
      return -1;
    }
    if(strncmp(recvbuf, "SUCCESS", 7)) return -1;
    LOG_INFO("OK");
    return 0;
  }

  int get(const std::string& key, std::string& value) {
    char sendbuf[kMaxMsg] = {0}, recvbuf[kMaxMsg] = {0};
    int bufsize = sprintf(sendbuf, "get %s", key.c_str());
    int fd;
    free_fds_.pop(fd);
    int n = write(fd, sendbuf, bufsize);
    if(n <= 0) {
      perror("write request error");
      free_fds_.push(fd);
      return -1;
    }
    n = readline(fd, recvbuf, kMaxMsg);
    free_fds_.push(fd);
    if(n <= 0) {
      perror("read response fail");
      return -1;
    }
    value = recvbuf;
    return 0;
  }

  int del(const std::string& key) {
    char sendbuf[kMaxMsg] = {0}, recvbuf[kMaxMsg];
    int bufsize = sprintf(sendbuf, "del %s", key.c_str());
    int fd;
    free_fds_.pop(fd);
    
    int n = write(fd, sendbuf, bufsize);
    if(n <= 0) {
      perror("write request error");
      free_fds_.push(fd);
      return -1;
    }
    n = readline(fd, recvbuf, kMaxMsg);
    free_fds_.push(fd);
    if(n <= 0) {
      perror("read response fail");
      return -1;
    }
    if(strncmp(recvbuf, "SUCCESS", 7)) return -1;
    return 0;
  }
};