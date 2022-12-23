#include "arena.h"
#include "test.h"
#include <condition_variable>
#include <thread>
#include <vector>

struct Noob {
  int a;
  double b;
};

class barrier_t {
  pthread_barrier_t b;

public:
  barrier_t() { pthread_barrier_init(&b, nullptr, 0); }
  barrier_t(unsigned int count) { pthread_barrier_init(&b, nullptr, count); }
  ~barrier_t() { pthread_barrier_destroy(&b); }

  void init(unsigned int count) { pthread_barrier_init(&b, nullptr, count); }
  void wait() { pthread_barrier_wait(&b); }
};

void TestBasic() {
  Arena &arena = Arena::getInstance();
  arena.Init(4096 * 16);

  std::vector<std::thread> threads;

  barrier_t bar0(4);
  barrier_t bar1(4);
  barrier_t bar2(4);
  barrier_t bar3(4);

  for (int i = 0; i < 4; i++) {
    threads.emplace_back([&bar0, &bar1, &bar2, &bar3, i]() {
      LOG_INFO("==== Alloc 1 ====");
      auto &arena = Arena::getInstance();
      std::vector<Noob *> noobs;
      for (int j = 0; j < 1024; j++) {
        auto *noob = arena.Alloc<Noob>();
        noobs.push_back(noob);
        noob->a = i * 10000 + j;
        noob->b = i * 10000 + j;
      }

      bar0.wait();
      EXPECT(arena.Alloc<Noob>() == nullptr, "");

      LOG_INFO("==== Validate 1 ====");
      for (int j = 0; j < 1024; j++) {
        auto *noob = noobs[j];
        EXPECT(noob->a == i * 10000 + j, "got %d expected %d", noob->a,
               i * 10000 + j);
        EXPECT(noob->b == i * 10000 + j, "got %lf expected %lf", noob->b,
               (double)(i * 10000 + j))
      }

      bar1.wait();
      LOG_INFO("==== Free ====");
      for (int j = 0; j < 1024; j++) {
        arena.Free(noobs[j]);
      }

      bar2.wait();
      LOG_INFO("==== Alloc 2 ===");
      noobs.clear();

      for (int j = 0; j < 1024; j++) {
        auto *noob = arena.Alloc<Noob>();
        EXPECT(noob != nullptr, "got null");
        noobs.push_back(noob);
        noob->a = i * 10001 + j;
        noob->b = i * 10001 + j;
      }

      bar3.wait();
      EXPECT(arena.Alloc<Noob *>() == nullptr, "");

      LOG_INFO("==== Validate 2 ====");
      for (int j = 0; j < 1024; j++) {
        auto *noob = noobs[j];
        EXPECT(noob->a == i * 10001 + j, "got %d expected %d", noob->a,
               i * 10001 + j);
        EXPECT(noob->b == i * 10001 + j, "got %lf expected %lf", noob->b,
               (double)(i * 10001 + j))
      }
    });
  }
  for (auto &thr : threads) {
    thr.join();
  }
  threads.clear();
}

int main() {
  TestBasic();
  return 0;
}