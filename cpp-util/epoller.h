#pragma once
#include "header.h"

enum NetWorkErrors {
  kNetWorkError = -2000,
  kNetWorkWouldBlock = -2001,
  kNetWorkInProgress = -2002,
};


struct InetAddr {
  struct sockaddr_in addr;

  bool operator==(const InetAddr& other) const {
    const struct sockaddr_in& other_addr = other.addr;

    if (addr.sin_addr.s_addr != other_addr.sin_addr.s_addr) {
      return false;
    }

    if (addr.sin_port != other_addr.sin_port) {
      return false;
    }

    return true;
  }

  bool operator<(const InetAddr& other) const {
    const struct sockaddr_in& other_addr = other.addr;

    if (addr.sin_addr.s_addr != other_addr.sin_addr.s_addr) {
      return addr.sin_addr.s_addr < other_addr.sin_addr.s_addr;
    }

    if (addr.sin_port != other_addr.sin_port) {
      return addr.sin_port < other_addr.sin_port;
    }

    return true;
  }

  InetAddr() { memset(&addr, 0, sizeof(addr)); }

  InetAddr(struct sockaddr_in _addr) { addr = _addr; }

  InetAddr(const char* ip, uint16_t port) {
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);
  }

  InetAddr(uint64_t addr_id) {
    uint32_t ip = (addr_id >> 16);
    uint16_t port = (addr_id & ((1 << 16) - 1));

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;
    addr.sin_port = htons(port);
  }

  InetAddr(uint32_t ip, uint16_t port) {
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;
    addr.sin_port = htons(port);
  }

  InetAddr(const std::string str_addr) {
    std::string rep = str_addr;
    for (auto& c : rep) {
      if (c == ':') {
        c = ' ';
      }
    }

    char ip[32];
    uint16_t port;
    sscanf(rep.c_str(), "%s%hu", ip, &port);

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);
  }

  std::string ToString() const {
    const char* ip = inet_ntoa(addr.sin_addr);
    uint16_t port = ntohs(addr.sin_port);

    char buf[32];
    snprintf(buf, 32, "%s:%hu", ip, port);

    return buf;
  }

  uint64_t GetAddrId() {
    uint32_t ip = addr.sin_addr.s_addr;
    uint16_t port = ntohs(addr.sin_port);
    return (uint64_t(ip) << 16) | port;
  }

  uint32_t GetIpByUint32() { return addr.sin_addr.s_addr; }
};

class TcpSocket {
 public:
  TcpSocket();
  TcpSocket(int fd, const InetAddr& local_addr, const InetAddr& peer_addr);
  ~TcpSocket();

  void Shutdown();

  // 1) Create socket. 2) Enable SO_REUSEADDR. 3) Close TCP negle algorithm.
  // 4) Set O_NONBLOCK.
  int InitSocket(bool non_blocked = true);

  int Bind(const InetAddr& local_addr);

  int Connect(const InetAddr& peer_addr);

  int Listen();

  // On success, the call return a nonnegative integer that is a descriptor for
  // the accepted socket.  On error, NetWork::kError is returned.
  int Accept(InetAddr& accepted_addr);

  int SetNonBlock(bool blocked);
  int CheckIfNonBlock(bool& blocked);

  int CheckIfValid();

  bool listened() { return listened_; }
  int fd() { return fd_; }

  bool BlockWrite(const char* buffer, uint32_t len);
  bool BlockRead(char* buffer, uint32_t len);

  std::string ToString();

  InetAddr peer_addr() { return peer_addr_; }
  InetAddr local_addr() { return local_addr_; }

 private:
  const int kBackLog = 1024;

  // The file descriptor will close when destruction.
  int fd_;

  bool listened_;
  bool connected_;
  bool non_blocked_;

  InetAddr local_addr_;
  InetAddr peer_addr_;

  TcpSocket(const TcpSocket&) = delete;
  TcpSocket& operator=(const TcpSocket&) = delete;
};

class FdObj;
class HandlerBase {
 public:
  virtual ~HandlerBase() {}
  virtual int HandleRead(FdObj* fd_obj) = 0;
  virtual int HandleWrite(FdObj* fd_obj) = 0;
};

class FdObj {
 private:
  int fd_;

  HandlerBase* handler_;
  int events_;

  bool readable_;
  bool writable_;
  bool broken_;

 public:
  static const int kDefaultEvents = EPOLLIN | EPOLLOUT | EPOLLET;

  FdObj(int fd, HandlerBase* handler, int events = kDefaultEvents)
      : fd_(fd),
        handler_(handler),
        events_(events),
        readable_(false),
        writable_(false),
        broken_(false) {}

  virtual ~FdObj() {}

  int fd() { return fd_; }

  int events() { return events_; }

  const bool& readable() { return readable_; }
  void set_readable(bool readable) { readable_ = readable; }
  const bool& writable() { return writable_; }
  void set_writable(bool writable) { writable_ = writable; }

  // Set by callers when the fd would be aborted.
  const bool& broken() {return broken_; }
  void set_broken(bool broken) { broken_ = broken; }

  HandlerBase* handler() { return handler_; }
  void set_handler(HandlerBase *handler) { handler_ = handler; }
};

class Poller {
 private:
  int epoll_fd_;

  uint32_t event_size_;
  epoll_event* epoll_events_;

 public:
  static const int kDefaultEventSize = 8096;

  Poller(int events_size = kDefaultEventSize);
  ~Poller();

  int Add(FdObj* fd_obj);
  int Remove(FdObj* fd_obj);
  int RemoveAndCloseFd(FdObj* fd_obj);
  int Modify(FdObj* fd_obj);

  void RunOnce(int msec);
};