#include "epoller.h"

#include "gtest/gtest.h"

class FooHandler : public HandlerBase {
 public:
  FooHandler() {
    read_count_ = 0;
    written_count_ = 0;
  }

  virtual ~FooHandler() {}

  virtual int HandleRead(FdObj* fd_obj) override {
    assert(fd_obj->readable());
    char buf[100] = {0};
    read(fd_obj->fd(), buf, 100);
    assert(std::string(buf) == "hello" || std::string(buf) == "hellohello");
    read_count_++;
    return 0;
  }

  virtual int HandleWrite(FdObj* fd_obj) override {
    assert(fd_obj->writable());
    char buf[100] = "hello";
    write(fd_obj->fd(), buf, strlen(buf));
    assert(std::string(buf) == "hello" || std::string(buf) == "hellohello");
    written_count_++;
    return 0;
  }
  const int& read_count() { return read_count_; }
  void set_read_count(int read_count) { read_count_ = read_count; }
  const int written_count() { return written_count_; }
  void set_written_count(int write_count) { written_count_ = write_count; } 

 private:
  int read_count_;
  int written_count_;
};

TEST(PollerTest, Basic) {
  Poller epoll_io;

  InetAddr addr1("127.0.0.1", 13768);

  TcpSocket svr;
  ASSERT_EQ(svr.InitSocket(), 0);
  ASSERT_EQ(svr.Bind(addr1), 0);
  ASSERT_EQ(svr.Listen(), 0);
  std::cout << svr.ToString() << std::endl;

  ASSERT_TRUE(svr.listened());

  TcpSocket client;
  InetAddr addr2("127.0.0.1", 13769);
  ASSERT_EQ(client.InitSocket(), 0);
  ASSERT_EQ(client.Bind(addr2), 0);
  ASSERT_EQ(client.Connect(addr1), kNetWorkInProgress);
  std::cout << client.ToString() << std::endl;

  InetAddr addr;
  int fd = svr.Accept(addr);
  ASSERT_GT(fd, 0);
  ASSERT_TRUE(addr == addr2);

  TcpSocket* connfd = new TcpSocket(fd, addr1, addr2);
  ASSERT_EQ(connfd->InitSocket(), 0);
  std::cout << connfd->ToString() << std::endl;

  FooHandler handler2;
  FooHandler handler3;

  auto fd_obj1 = new FdObj(client.fd(), &handler2);
  auto fd_obj2 = new FdObj(connfd->fd(), &handler3);
  ASSERT_EQ(epoll_io.Add(fd_obj1), 0);
  ASSERT_EQ(epoll_io.Add(fd_obj2), 0);
  ASSERT_EQ(epoll_io.Modify(fd_obj2), 0);

  epoll_io.RunOnce(1);
  epoll_io.RunOnce(1);
  epoll_io.RunOnce(1);
  epoll_io.RunOnce(1);

  ASSERT_EQ(handler3.written_count(), 4);
  ASSERT_EQ(handler2.written_count(), 4);

  ASSERT_EQ(epoll_io.RemoveAndCloseFd(fd_obj1), 0);

  epoll_io.RunOnce(1);
  ASSERT_EQ(handler2.written_count(), 4);
  ASSERT_EQ(handler3.written_count(), 5);
}
