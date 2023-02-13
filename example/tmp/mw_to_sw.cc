#include "condvar.h"
#include "slice.h"
#include "singleton.h"
#include <deque>
#include <thread>
#include "Random.h"
#include <atomic>
#include <logging.h>

struct Writer {
  explicit Writer(std::mutex *mu) : cv(mu), done(false) {}
  CondVar cv;
  Slice data;
  bool done;
};
char g_buf[1024];
const int userNum = 8;
const int userDataNum = 10'000;
std::atomic<int> count{0};
Random rander;

bool do_batched;

struct WriterMng: public Singleton<WriterMng> {
  std::deque<Writer*> writers;
  std::mutex mu; // protects deque
};

void write(int tid, const std::string& user_data) {
  Writer w(&WriterMng::GetInstance()->mu);
  w.data = Slice("tid" + std::to_string(tid) + user_data);
  // lock protecting deque
  std::unique_lock<std::mutex> l(WriterMng::GetInstance()->mu);
  WriterMng::GetInstance()->writers.push_back(&w);
  // wait to be first
  while(!w.done && &w != WriterMng::GetInstance()->writers.front()) {
    w.cv.Wait();
  }
  if(w.done) {
    return; // already done
  }
  Writer* last_writer = &w;
  // do batch
  int pos = 0;
  auto iter = WriterMng::GetInstance()->writers.begin();
  for(; iter != WriterMng::GetInstance()->writers.end(); ++iter) {
    Writer *wr = *iter;
    if(wr->data.size()) {
      memcpy(g_buf+pos, wr->data.data(), wr->data.size());
      pos += wr->data.size();
    }
    last_writer = wr;
  }
  g_buf[pos] = '\0';
  // no need protect deque now
  WriterMng::GetInstance()->mu.unlock();
  // other threads data into deque, but cv.Wait(); because not the first
  // and the data in current does not process yet, so cannot pop out.
  // maintain last_writer to solve this
  printf("%s\n", g_buf);
  WriterMng::GetInstance()->mu.lock();
  
  //pop a batch of writer have doned
  while(true) {
    Writer *ready = WriterMng::GetInstance()->writers.front();
    WriterMng::GetInstance()->writers.pop_front();
    count.fetch_add(1, std::memory_order_relaxed);
    // not the first writer
    if(ready != &w) {
      ready->done = true;
      ready->cv.Signal();
      do_batched = true;
    }
    // last_writer+1 is not processed
    if (ready == last_writer) break;
  }

  // Notify new head of write queue, and process header
  if(!WriterMng::GetInstance()->writers.empty()) {
    WriterMng::GetInstance()->writers.front()->cv.Signal();
  }
}

void user_func(int tid) {
  for(int i=0; i<userDataNum; i++) {
    std::string user_data = std::to_string(rander.rand32());
    write(tid, user_data);
  }
}


int main() {
  std::thread t[userNum];
  for(int i=0; i<userNum; ++i) {
    t[i] = std::thread(user_func, i);
  }
  for(int i=0; i<userNum; ++i) {
    t[i].join();
  }
  
  LOG_ASSERT(count.load(std::memory_order_relaxed) == userNum*userDataNum, "%d", count.load(std::memory_order_relaxed));
  if(do_batched) {
    printf("--------batched-----------\n");
  }
  return 0;
}