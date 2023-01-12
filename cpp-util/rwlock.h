/**
 * very fast read write lock, cpu friendly
 * learn from nginx
 * Copyright (c) msk 2022
 */
#include <atomic>
#include <climits>
extern "C"
{
#include <sched.h>
}

class Rwlock
{
public:
  bool tryWLock()
  {
    return lock_.load(std::memory_order_acquire) == 0 && lock_.compare_exchange_strong(kNone, kWlock_);
  }
  void WLock()
  {
    int n, i;
    for (;;)
    {
      if (lock_.load(std::memory_order_acquire) == 0 && lock_.compare_exchange_strong(kNone, kWlock_))
      {
        return;
      }
      for (n = 1; n < 2048; n <<= 1)
      {
        for (i = 0; i < n; i++)
        {
          __asm__("pause");
        }
        if (lock_.load(std::memory_order_acquire) == 0 && lock_.compare_exchange_strong(kNone, kWlock_))
        {
          return;
        }
      }
      sched_yield();
    }
  }
  void RLock()
  {
    int readers, n, i;
    for (;;)
    {
      readers = lock_.load(std::memory_order_acquire);
      if (readers != kWlock_ && lock_.compare_exchange_strong(readers, readers + 1))
      {
        return;
      }
      for (n = 1; n < 2048; n <<= 1)
      {
        for (i = 0; i < n; i++)
        {
          __asm__("pause");
        }
        readers = lock_.load(std::memory_order_acquire);
        if (readers != kWlock_ && lock_.compare_exchange_strong(readers, readers + 1))
        {
          return;
        }
      }
      sched_yield();
    }
  }
  bool tryRLock()
  {
    int readers = lock_.load(std::memory_order_acquire);
    return readers != kWlock_ && lock_.compare_exchange_strong(readers, readers + 1);
  }
  void Unlock()
  {
    int readers;
    readers = lock_.load(std::memory_order_acquire);
    if (readers == kWlock_) {
        lock_.store(0,std::memory_order_relaxed);
        return;
    }
    for (;;)
    {
      if (lock_.compare_exchange_strong(readers, readers - 1))
      {
        return;
      }
      readers = lock_.load(std::memory_order_acquire);
    }
  }

private:
  std::atomic<int> lock_;
  static int kWlock_;
  static int kNone;
};
int Rwlock::kWlock_ = INT_MAX - 1;
int Rwlock::kNone = 0;