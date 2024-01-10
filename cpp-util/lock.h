#pragma once
#include <atomic>
#include <thread>
#include <cstdlib>
#include <climits>
#include <condition_variable>
#include <mutex>
#include <sched.h>
#define CACHE_LINE_SIZE 64

// SpinMutex has very low overhead for low-contention cases.  Method names
// are chosen so you can use std::unique_lock or std::lock_guard with it.
class SpinMutex {
 public:
  SpinMutex() : locked_(false) {}

  bool try_lock() {
    auto currently_locked = locked_.load(std::memory_order_relaxed);
    return !currently_locked &&
           locked_.compare_exchange_weak(currently_locked, true,
                                         std::memory_order_acquire,
                                         std::memory_order_relaxed);
  }

  void lock() {
    for (size_t tries = 0;; ++tries) {
      if (try_lock()) {
        // success
        break;
      }
      asm volatile("pause");
      if (tries > 100) {
        std::this_thread::yield();
      }
    }
  }

  void unlock() { locked_.store(false, std::memory_order_release); }

 private:
  std::atomic<bool> locked_;
};

class RWMutex {
 public:
  RWMutex() {
    pthread_rwlock_init(&mu_, nullptr);
  }
  // No copying allowed
  RWMutex(const RWMutex&) = delete;
  void operator=(const RWMutex&) = delete;

  ~RWMutex() {
    pthread_rwlock_destroy(&mu_);
  }

  void ReadLock() {
    pthread_rwlock_rdlock(&mu_);
  }
  void WriteLock() {
    pthread_rwlock_wrlock(&mu_);
  }
  void ReadUnlock() {
    pthread_rwlock_unlock(&mu_);
  }
  void WriteUnlock() {
    pthread_rwlock_unlock(&mu_);
  }

 private:
  pthread_rwlock_t mu_;  // the underlying platform mutex
};

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
 * 读写均衡，写会等待所有读锁释放；读会等待写锁释放，类似nginx的读写锁
*/
class CASRWLock {
  public:
  CASRWLock(): lock(0) {}
  void Rlock() {
    uint64_t i, n;
    uint64_t old_readers;
    while(true) {
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
    while(true) {
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
    while(true) {
      old_readers = lock;
      if (__sync_bool_compare_and_swap(&lock, old_readers, old_readers - 1)) {
          return;
      }
    }
  }
private:
  static const uint64_t SPIN = 2048;
  static const uint64_t WLOCK = ((unsigned long)-1);
  uint64_t lock;
};

template <class T>
struct alignas(CACHE_LINE_SIZE) LockData {
  T lock_;
};

// A striped Lock. This offers the underlying lock striping 
// similar to that of ConcurrentHashMap in a reusable form
// extends it for semaphores and read-write locks.
// increasing the granularity of a single lock and 
// allowing independent operations to lock different stripes and
// proceed concurrently, instead of creating contention for a single lock.
template <class T, class P>
class Striped {
 public:
  Striped(size_t stripes, std::function<uint64_t(const P &)> hash_func)
      : stripes_(stripes), hash_func_(hash_func) {
    void *ptr;
    posix_memalign(&ptr, CACHE_LINE_SIZE, sizeof(LockData<T>) * stripes);
    locks_ = reinterpret_cast<LockData<T> *>(ptr);
    for (size_t i = 0; i < stripes; i++) {
      new (&locks_[i]) LockData<T>();
    }
  }

  virtual ~Striped() {
    if (locks_ != nullptr) {
      assert(stripes_ > 0);
      for (size_t i = 0; i < stripes_; i++) {
        locks_[i].~LockData<T>();
      }
      free(locks_);
    }
  }

  T *get(const P &key) {
    uint64_t h = hash_func_(key);
    size_t index = h % stripes_;
    return &reinterpret_cast<LockData<T> *>(&locks_[index])->lock_;
  }

 private:
  size_t stripes_;
  LockData<T> *locks_;
  std::function<uint64_t(const P &)> hash_func_;
};