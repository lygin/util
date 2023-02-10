#pragma once
#include <atomic>
#include <thread>
#include <cstdlib>

#define CACHE_LINE_SIZE 128

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

// Acquire a ReadLock on the specified RWMutex.
// The Lock will be automatically released when the
// object goes out of scope.
class ReadLock {
 public:
  explicit ReadLock(RWMutex *mu) : mu_(mu) { this->mu_->ReadLock(); }
  // No copying allowed
  ReadLock(const ReadLock &) = delete;
  void operator=(const ReadLock &) = delete;

  ~ReadLock() { this->mu_->ReadUnlock(); }

 private:
  RWMutex *const mu_;
};

// Acquire a WriteLock on the specified RWMutex.
// The Lock will be automatically released then the
// object goes out of scope.
class WriteLock {
 public:
  explicit WriteLock(RWMutex *mu) : mu_(mu) { this->mu_->WriteLock(); }
  // No copying allowed
  WriteLock(const WriteLock &) = delete;
  void operator=(const WriteLock &) = delete;

  ~WriteLock() { this->mu_->WriteUnlock(); }

 private:
  RWMutex *const mu_;
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