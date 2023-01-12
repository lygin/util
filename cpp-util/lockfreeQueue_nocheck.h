/**
 * lock free mpmc and spsc ring Queue
 * cache friendly, bounded
*/
#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>

namespace mpmc
{
  /* 缓存行对齐 */
  static constexpr size_t kCacheLineSize = 64;

  template <typename T>
  struct AlignedAllocator
  {
    using value_type = T;
    T *allocate(std::size_t n)
    {
      if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
      {
        throw std::bad_array_new_length();
      }
      T *p;
      if (posix_memalign(reinterpret_cast<void **>(&p), alignof(T),
                         sizeof(T) * n) != 0)
      {
        throw std::bad_alloc();
      }
      return p;
    }
    void deallocate(T *p, std::size_t)
    {
      free(p);
    }
  };

  template <typename T>
  struct Slot
  {
    ~Slot() noexcept
    {
      if (turn & 1)
      {
        destroy();
      }
    }

    template <typename... Args>
    void construct(Args &&...args) noexcept
    {
      new (&storage) T(std::forward<Args>(args)...);
    }

    void destroy() noexcept
    {
      reinterpret_cast<T *>(&storage)->~T();
    }

    T &&move() noexcept { return reinterpret_cast<T &&>(storage); }

    alignas(kCacheLineSize) std::atomic<size_t> turn = {0}; // alignas(64) c++11
    typename std::aligned_storage<sizeof(T), alignof(T)>::type storage;
  };

  template <typename T, typename Allocator = AlignedAllocator<Slot<T>>>
  class Ring
  {
  private:

  public:
    explicit Ring(const size_t capacity,
                   const Allocator &allocator = Allocator())
        : capacity_(capacity), allocator_(allocator), head_(0), tail_(0)
    {
      if (capacity_ < 1)
      {
        throw std::invalid_argument("capacity < 1");
      }
      slots_ = allocator_.allocate(capacity_ + 1);
      if (reinterpret_cast<size_t>(slots_) % alignof(Slot<T>) != 0)
      {
        allocator_.deallocate(slots_, capacity_ + 1);
        throw std::bad_alloc();
      }
      for (size_t i = 0; i < capacity_; ++i)
      {
        new (&slots_[i]) Slot<T>();
      }
    }

    ~Ring() noexcept
    {
      for (size_t i = 0; i < capacity_; ++i)
      {
        slots_[i].~Slot();
      }
      allocator_.deallocate(slots_, capacity_ + 1);
    }

    Ring(const Ring &) = delete;
    Ring &operator=(const Ring &) = delete;

    template <typename... Args>
    void emplace(Args &&...args) noexcept
    {
      auto const head = head_.fetch_add(1);
      auto &slot = slots_[idx(head)];
      while (turn(head) * 2 != slot.turn.load(std::memory_order_acquire))
        ;
      slot.construct(std::forward<Args>(args)...);
      slot.turn.store(turn(head) * 2 + 1, std::memory_order_release);
    }

    template <typename... Args>
    bool try_emplace(Args &&...args) noexcept
    {
      auto head = head_.load(std::memory_order_acquire);
      for (;;)
      {
        auto &slot = slots_[idx(head)];
        if (turn(head) * 2 == slot.turn.load(std::memory_order_acquire))
        {
          if (head_.compare_exchange_strong(head, head + 1))
          {
            slot.construct(std::forward<Args>(args)...);
            slot.turn.store(turn(head) * 2 + 1, std::memory_order_release);
            return true;
          }
        }
        else
        {
          auto const prevHead = head;
          head = head_.load(std::memory_order_acquire);
          if (head == prevHead)
          {
            return false;
          }
        }
      }
    }

    void push(const T &v) noexcept
    {
      emplace(v);
    }

    template <typename P,
              typename = typename std::enable_if<
                  std::is_nothrow_constructible<T, P &&>::value>::type>
    void push(P &&v) noexcept
    {
      emplace(std::forward<P>(v));
    }

    bool try_push(const T &v) noexcept
    {
      return try_emplace(v);
    }

    template <typename P,
              typename = typename std::enable_if<
                  std::is_nothrow_constructible<T, P &&>::value>::type>
    bool try_push(P &&v) noexcept
    {
      return try_emplace(std::forward<P>(v));
    }

    void pop(T &v) noexcept
    {
      auto const tail = tail_.fetch_add(1);
      auto &slot = slots_[idx(tail)];
      while (turn(tail) * 2 + 1 != slot.turn.load(std::memory_order_acquire))
        ;
      v = slot.move();
      slot.destroy();
      slot.turn.store(turn(tail) * 2 + 2, std::memory_order_release);
    }

    bool try_pop(T &v) noexcept
    {
      auto tail = tail_.load(std::memory_order_acquire);
      for (;;)
      {
        auto &slot = slots_[idx(tail)];
        if (turn(tail) * 2 + 1 == slot.turn.load(std::memory_order_acquire))
        {
          if (tail_.compare_exchange_strong(tail, tail + 1))
          {
            v = slot.move();
            slot.destroy();
            slot.turn.store(turn(tail) * 2 + 2, std::memory_order_release);
            return true;
          }
        }
        else
        {
          auto const prevTail = tail;
          tail = tail_.load(std::memory_order_acquire);
          if (tail == prevTail)
          {
            return false;
          }
        }
      }
    }
    ptrdiff_t size() const noexcept
    {
      return static_cast<ptrdiff_t>(head_.load(std::memory_order_relaxed) -
                                    tail_.load(std::memory_order_relaxed));
    }

    bool empty() const noexcept { return size() <= 0; }

  private:
    constexpr size_t idx(size_t i) const noexcept { return i % capacity_; }

    constexpr size_t turn(size_t i) const noexcept { return i / capacity_; }

  private:
    const size_t capacity_;
    Slot<T> *slots_;
    Allocator allocator_;
    alignas(kCacheLineSize) std::atomic<size_t> head_;
    alignas(kCacheLineSize) std::atomic<size_t> tail_;
  };
} // namespace mpmc

template <typename T, typename Allocator = std::allocator<T>>
class SPSCRing {
public:
  explicit SPSCRing(const size_t capacity,
                     const Allocator &allocator = Allocator())
      : capacity_(capacity), allocator_(allocator) {
    if (capacity_ < 1) {
      capacity_ = 1;
    }
    capacity_++;
    if (capacity_ > SIZE_MAX - 2 * kPadding) {
      capacity_ = SIZE_MAX - 2 * kPadding;
    }

    slots_ = std::allocator_traits<Allocator>::allocate(
        allocator_, capacity_ + 2 * kPadding);
  }

  ~SPSCRing() {
    while (front()) {
      pop();
    }
    std::allocator_traits<Allocator>::deallocate(allocator_, slots_,
                                                 capacity_ + 2 * kPadding);
  }

  SPSCRing(const SPSCRing &) = delete;
  SPSCRing &operator=(const SPSCRing &) = delete;

  template <typename... Args>
  void emplace(Args &&...args) noexcept {
    auto const writeIdx = writeIdx_.load(std::memory_order_relaxed);
    auto nextWriteIdx = writeIdx + 1;
    if (nextWriteIdx == capacity_) {
      nextWriteIdx = 0;
    }
    while (nextWriteIdx == readIdxCache_) {
      readIdxCache_ = readIdx_.load(std::memory_order_acquire);
    }
    new (&slots_[writeIdx + kPadding]) T(std::forward<Args>(args)...);
    writeIdx_.store(nextWriteIdx, std::memory_order_release);
  }

  template <typename... Args>
  bool try_emplace(Args &&...args) noexcept {
    auto const writeIdx = writeIdx_.load(std::memory_order_relaxed);
    auto nextWriteIdx = writeIdx + 1;
    if (nextWriteIdx == capacity_) {
      nextWriteIdx = 0;
    }
    if (nextWriteIdx == readIdxCache_) {
      readIdxCache_ = readIdx_.load(std::memory_order_acquire);
      if (nextWriteIdx == readIdxCache_) {
        return false;
      }
    }
    new (&slots_[writeIdx + kPadding]) T(std::forward<Args>(args)...);
    writeIdx_.store(nextWriteIdx, std::memory_order_release);
    return true;
  }

  void push(const T &v) noexcept {
    emplace(v);
  }

  template <typename P, typename = typename std::enable_if<
                            std::is_constructible<T, P &&>::value>::type>
  void push(P &&v) noexcept {
    emplace(std::forward<P>(v));
  }

  bool try_push(const T &v) noexcept {
    return try_emplace(v);
  }

  template <typename P, typename = typename std::enable_if<
                            std::is_constructible<T, P &&>::value>::type>
  bool try_push(P &&v) noexcept {
    return try_emplace(std::forward<P>(v));
  }

  T *front() noexcept {
    auto const readIdx = readIdx_.load(std::memory_order_relaxed);
    if (readIdx == writeIdxCache_) {
      writeIdxCache_ = writeIdx_.load(std::memory_order_acquire);
      if (writeIdxCache_ == readIdx) {
        return nullptr;
      }
    }
    return &slots_[readIdx + kPadding];
  }

  void pop() noexcept {
    auto const readIdx = readIdx_.load(std::memory_order_relaxed);
    assert(writeIdx_.load(std::memory_order_acquire) != readIdx);
    slots_[readIdx + kPadding].~T();
    auto nextReadIdx = readIdx + 1;
    if (nextReadIdx == capacity_) {
      nextReadIdx = 0;
    }
    readIdx_.store(nextReadIdx, std::memory_order_release);
  }

  size_t size() const noexcept {
    std::ptrdiff_t diff = writeIdx_.load(std::memory_order_acquire) -
                          readIdx_.load(std::memory_order_acquire);
    if (diff < 0) {
      diff += capacity_;
    }
    return static_cast<size_t>(diff);
  }

  bool empty() const noexcept {
    return writeIdx_.load(std::memory_order_acquire) ==
           readIdx_.load(std::memory_order_acquire);
  }

  size_t capacity() const noexcept { return capacity_ - 1; }

private:

  static constexpr size_t kCacheLineSize = 64;
  static constexpr size_t kPadding = (kCacheLineSize - 1) / sizeof(T) + 1;

private:
  size_t capacity_;
  T *slots_;
  Allocator allocator_;
  alignas(kCacheLineSize) std::atomic<size_t> writeIdx_ = {0};
  alignas(kCacheLineSize) size_t readIdxCache_ = 0;
  alignas(kCacheLineSize) std::atomic<size_t> readIdx_ = {0};
  alignas(kCacheLineSize) size_t writeIdxCache_ = 0;

  char padding_[kCacheLineSize - sizeof(writeIdxCache_)];
};

template <typename T,
          typename Allocator = mpmc::AlignedAllocator<mpmc::Slot<T>>>
using MPMCRing = mpmc::Ring<T, Allocator>;