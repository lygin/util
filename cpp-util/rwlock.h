#pragma once

#include <climits>
#include <condition_variable>  // NOLINT
#include <mutex>               // NOLINT

/**
 * @brief 写抢占
 * 写进入后，会等待之前进入的读者读完，后面进入的读者会阻塞
 */
class RWLock {
  using mutex_t = std::mutex;
  using cond_t = std::condition_variable;
  static const uint32_t MAX_READERS = UINT_MAX;

 public:
  RWLock() = default;
  ~RWLock() { std::lock_guard<mutex_t> guard(mutex_); }

  RWLock(const RWLock&) = delete;
  RWLock &operator=(const RWLock&) = delete;

  /**
   * Acquire a write latch.
   */
  void WLock() {
    std::unique_lock<mutex_t> latch(mutex_);
    while (writer_entered_) {
      reader_.wait(latch);
    }
    writer_entered_ = true;
    while (reader_count_ > 0) {
      writer_.wait(latch);
    }
  }

  /**
   * Release a write latch.
   */
  void WUnlock() {
    std::lock_guard<mutex_t> guard(mutex_);
    writer_entered_ = false;
    reader_.notify_all();
  }

  /**
   * Acquire a read latch.
   */
  void RLock() {
    std::unique_lock<mutex_t> latch(mutex_);
    while (writer_entered_ || reader_count_ == MAX_READERS) {
      reader_.wait(latch);
    }
    reader_count_++;
  }

  /**
   * Release a read latch.
   */
  void RUnlock() {
    std::lock_guard<mutex_t> guard(mutex_);
    reader_count_--;
    if (writer_entered_) {
      if (reader_count_ == 0) {
        writer_.notify_one();
      }
    } else {
      if (reader_count_ == MAX_READERS - 1) {
        reader_.notify_one();
      }
    }
  }

 private:
  mutex_t mutex_;
  cond_t writer_;
  cond_t reader_;
  uint32_t reader_count_{0};
  bool writer_entered_{false};
};

/**
 * 读写均衡，写会等待所有读锁释放；读会等待写锁释放
*/
class CASRWLock {
  public:
  CASRWLock(): lock(0) {}
  void Rlock() {
    uint64_t i, n;
    uint64_t old_readers;
    for ( ;; ) {
      old_readers = lock;
      if (old_readers != WLOCK && __sync_bool_compare_and_swap(&lock, old_readers, old_readers + 1)) {
          return;
      }
      for (n = 1; n < SPIN; n <<= 1) {
        for (i = 0; i < n; i++) {
            __asm__ ("pause");
        }
        old_readers = lock;
        if (old_readers != WLOCK && __sync_bool_compare_and_swap(&lock, old_readers, old_readers + 1)) {
            return;
        }
      }
      sched_yield();
    }
  }
  void Wlock() {
    uint64_t i, n;
    for ( ;; ) {
      if (lock == 0 && __sync_bool_compare_and_swap(&lock, 0, WLOCK)) {
          return;
      }  
      for (n = 1; n < SPIN; n <<= 1) {
        for (i = 0; i < n; i++) {
            __asm__ ("pause");
        }
        if (lock == 0 &&  __sync_bool_compare_and_swap(&lock, 0, WLOCK)) {
            return;
        }
      }
      sched_yield();
    }
  }
  void Unlock() {
    uint64_t old_readers;
    old_readers = lock;
    if (old_readers == WLOCK) {
      lock = 0;
      return;
    }
    for ( ;; ) {
      if (__sync_bool_compare_and_swap(&lock, old_readers, old_readers - 1)) {
          return;
      }
      old_readers = lock;
    }
  }
private:
  static const uint64_t SPIN = 2048;
  static const uint64_t WLOCK = ((unsigned long)-1);
  uint64_t lock;
};
