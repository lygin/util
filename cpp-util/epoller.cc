#include "epoller.h"
#include "logging.h"

TcpSocket::TcpSocket() {
  fd_ = -1;
  listened_ = false;
  connected_ = false;
  non_blocked_ = false;
}

TcpSocket::TcpSocket(int fd, const InetAddr& local_addr,
                     const InetAddr& peer_addr) {
  fd_ = fd;
  listened_ = false;
  connected_ = true;
  non_blocked_ = false;
  local_addr_ = local_addr;
  peer_addr_ = peer_addr;
}

TcpSocket::~TcpSocket() {
  if (fd_ != -1) {
    int ret = close(fd_);
    if (ret < 0) {
      LOG_ERROR("close failed fd %d errno [%s]", fd_, strerror(errno));
    }
  }
}

void TcpSocket::Shutdown() {
  if (fd_ == -1) {
    return;
  }
  int ret = shutdown(fd_, SHUT_RDWR);
  if (ret < 0) {
    LOG_ERROR("shutdown failed fd %d errno [%s]", fd_, strerror(errno));
  }
}

int TcpSocket::InitSocket(bool non_blocked) {
  int ret, val = 1;

  if (fd_ == -1) {
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ == -1) {
      LOG_ERROR("socket failed fd %d errno [%s]", fd_, strerror(errno));
      return kNetWorkError;
    }
  }

  // Enable SO_REUSEADDR.
  ret = setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
  if (ret == -1) {
    LOG_ERROR("setsockopt failed fd %d errno [%s]", fd_,
                      strerror(errno));
    return kNetWorkError;
  }

  // Close TCP negle algorithm.
  ret = setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
  if (ret == -1) {
    LOG_ERROR("setsockopt failed fd %d errno [%s]", fd_,
                      strerror(errno));
    return kNetWorkError;
  }

  // Set O_NONBLOCK.
  ret = SetNonBlock(non_blocked);
  if (ret == -1) {
    LOG_ERROR("SetNonBlock failed");
    return kNetWorkError;
  }

  return 0;
}

int TcpSocket::Bind(const InetAddr& local_addr) {
  assert(fd_ >= 0);
  assert(!listened_);
  assert(!connected_);

  struct sockaddr* addr = (struct sockaddr*)&local_addr.addr;
  socklen_t len = sizeof(*addr);

  int ret = bind(fd_, addr, len);
  if (ret == -1) {
    LOG_ERROR("bind failed fd_ %d addr %s errno [%s]", fd_,
                      local_addr.ToString().c_str(), strerror(errno));
    return kNetWorkError;
  }

  local_addr_ = local_addr;

  return 0;
}

int TcpSocket::Connect(const InetAddr& peer_addr) {
  assert(fd_ >= 0);
  assert(!listened_);
  assert(!connected_);

  const struct sockaddr_in* addr = &peer_addr.addr;
  socklen_t len = sizeof(*addr);

  peer_addr_ = peer_addr;

  int ret = connect(fd_, (struct sockaddr*)addr, len);
  if (ret == -1) {
    if (errno == EINPROGRESS) {
      connected_ = true;
      return kNetWorkInProgress;
    }

    LOG_ERROR("connect failed fd_ %d local %s peer %s errno [%s]", fd_,
                      local_addr_.ToString().c_str(),
                      peer_addr_.ToString().c_str(), strerror(errno));
    return kNetWorkError;
  }
  connected_ = true;

  return 0;
}

int TcpSocket::Listen() {
  assert(fd_ >= 0);
  assert(!listened_);
  assert(!connected_);

  int ret = listen(fd_, kBackLog);
  if (ret == -1) {
    LOG_ERROR("listen failed fd_ %d local %s errno [%s]", fd_,
                      local_addr_.ToString().c_str(), strerror(errno));
    return kNetWorkError;
  }
  listened_ = true;

  return 0;
}

int TcpSocket::Accept(InetAddr& accepted_addr) {
  assert(listened_);

  int accepted_fd;
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);

  while (1) {
    accepted_fd = accept(fd_, (struct sockaddr*)(&addr), &len);
    if (accepted_fd == -1) {
      if (errno == EINTR) {
        continue;
      } else if (errno == EAGAIN) {
        return kNetWorkWouldBlock;
      }

      LOG_ERROR("accept failed fd_ %d local %s errno [%s]", fd_,
                        local_addr_.ToString().c_str(), strerror(errno));
      return kNetWorkError;
    } else {
      accepted_addr = InetAddr(addr);
      break;
    }
  }

  return accepted_fd;
}

int TcpSocket::SetNonBlock(bool non_blocked) {
  assert(fd_ >= 0);

  int flag = fcntl(fd_, F_GETFL, 0);
  if (flag == -1) {
    LOG_ERROR("fcntl failed errno [%s]", strerror(errno));
    return kNetWorkError;
  }

  flag |= O_NONBLOCK;
  if (!non_blocked) {
    flag ^= O_NONBLOCK;
  }

  int ret = fcntl(fd_, F_SETFL, flag);
  if (ret == -1) {
    LOG_ERROR("fcntl failed errno [%s]", strerror(errno));
    return kNetWorkError;
  }

  non_blocked_ = non_blocked;

  return 0;
}

int TcpSocket::CheckIfNonBlock(bool& non_blocked) {
  assert(fd_ >= 0);

  int flag = fcntl(fd_, F_GETFL, 0);
  if (flag == -1) {
    LOG_ERROR("fcntl failed errno [%s]", strerror(errno));
    return kNetWorkError;
  }

  non_blocked = flag & O_NONBLOCK;
  return 0;
}

int TcpSocket::CheckIfValid() {
  assert(fd_ >= 0);

  int error = 0;
  socklen_t len = sizeof(error);
  int ret = getsockopt(fd_, SOL_SOCKET, SO_ERROR, &error, &len);
  if (ret == -1) {
    LOG_ERROR("getsockopt failed errno [%s]", strerror(errno));
    return kNetWorkError;
  }

  if (error != 0) {
    LOG_ERROR("getsockopt succ errno [%s]", strerror(errno));
    return kNetWorkError;
  }

  return 0;
}

bool TcpSocket::BlockWrite(const char* buffer, uint32_t len) {
  if (non_blocked_) {
    LOG_ERROR("fd %d is non_block");
    return false;
  }

  uint32_t offset = 0;
  while (offset < len) {
    int ret = write(fd_, buffer + offset, len - offset);
    if (ret < 0) {
      if (errno == EINTR) {
        continue;
      }
      LOG_ERROR("write socket: %s errno [%s]", ToString().c_str(),
                        strerror(errno));
      return false;
    }
    offset += ret;
  }

  return true;
}

bool TcpSocket::BlockRead(char* buffer, uint32_t len) {
  if (non_blocked_) {
    LOG_ERROR("fd %d is non_block");
    return false;
  }

  uint32_t offset = 0;
  while (offset < len) {
    int ret = read(fd_, buffer + offset, len - offset);
    if (ret < 0) {
      if (errno == EINTR) {
        continue;
      }
      LOG_ERROR("read socket: %s errno [%s]", ToString().c_str(),
                        strerror(errno));
      return false;
    }
    if (ret == 0) {
      LOG_ERROR("read socket: %s errno [%s]", ToString().c_str(),
                        strerror(errno));
      return false;
    }
    offset += ret;
  }

  return true;
}

std::string TcpSocket::ToString() {
  char buf[100];
  snprintf(buf, 100, "fd %d li %u co %u nbl %u local %s peer %s", fd_,
           listened_, connected_, non_blocked_, local_addr_.ToString().c_str(),
           peer_addr_.ToString().c_str());
  return buf;
}

Poller::Poller(int event_size) {
  event_size_ = event_size;
  epoll_events_ = (epoll_event*)calloc(event_size, sizeof(epoll_event));
  assert(epoll_events_ != NULL);

  epoll_fd_ = epoll_create(1024);
  if (epoll_fd_ == -1) {
    LOG_ERROR("epoll_create failed epoll_fd %d errno [%s]", epoll_fd_,
                      strerror(errno));
    assert(false);
  }
}

Poller::~Poller() {
  free(epoll_events_);

  if (epoll_fd_ >= 0) {
    close(epoll_fd_);
  }
}

int Poller::Add(FdObj* fd_obj) {
  int fd = fd_obj->fd();

  epoll_event ev = {0};
  ev.events = fd_obj->events();
  ev.data.ptr = static_cast<void*>(fd_obj);

  int ret = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
  if (ret == -1) {
    LOG_ERROR("epoll_ctl failed fd %d errno [%s]", fd, strerror(errno));
    return kNetWorkError;
  }

  return 0;
}

int Poller::Remove(FdObj* fd_obj) {
  int fd = fd_obj->fd();

  int ret = epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, NULL);
  if (ret == -1) {
    LOG_ERROR("epoll_ctl failed fd %d errno [%s]", fd, strerror(errno));
    return kNetWorkError;
  }

  return 0;
}

int Poller::RemoveAndCloseFd(FdObj* fd_obj) {
  int ret;
  int fd = fd_obj->fd();

  ret = Remove(fd_obj);
  if (ret != 0) {
    LOG_ERROR("Remove ret %d", ret);
    return kNetWorkError;
  }

  ret = close(fd);
  if (ret == -1) {
    LOG_ERROR("close failed fd %d errno [%s]", fd, strerror(errno));
    return kNetWorkError;
  }

  return 0;
}

int Poller::Modify(FdObj* fd_obj) {
  int fd = fd_obj->fd();

  epoll_event ev = {0};
  ev.events = fd_obj->events();
  ev.data.ptr = static_cast<void*>(fd_obj);

  int ret = epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
  if (ret == -1) {
    LOG_ERROR("epoll_ctl failed fd %d errno [%s]", fd, strerror(errno));
    return kNetWorkError;
  }

  return 0;
}

void Poller::RunOnce(int msec) {
  int num, ret;

  while (1) {
    num = epoll_wait(epoll_fd_, epoll_events_, event_size_, msec);
    if (num == -1) {
      if (errno == EINTR) {
        continue;
      }

      LOG_ERROR("epoll_wait failed fd %d errno [%s]", epoll_fd_,
                        strerror(errno));
      assert(false);
    }
    break;
  }

  // With ET epoll, all the events(i.e. readable and writable)
  // can be shown upon receipt of one event(i.e. readable).

  for (int i = 0; i < num; ++i) {
    int events = epoll_events_[i].events;
    FdObj* fd_obj = static_cast<FdObj*>(epoll_events_[i].data.ptr);
    HandlerBase* handler = fd_obj->handler();
    assert(handler != NULL);

    if ((events & EPOLLIN) || (events & EPOLLERR) || (events & EPOLLHUP)) {
      fd_obj->set_readable(true);
      ret = handler->HandleRead(fd_obj);
      if (ret != 0) {
        continue;
      }
    }

    if (events & EPOLLOUT) {
      fd_obj->set_writable(true);
      ret = handler->HandleWrite(fd_obj);
      if (ret != 0) {
        continue;
      }
    }
  }
}
