/**
 * Usage: 
 * MPMC: https://github.com/rigtorp/MPMCQueue
 * SPSC: https://github.com/rigtorp/SPSCQueue
*/
#pragma once

#include <atomic>
#include <cassert>
#include <cstddef> // offsetof
#include <limits>
#include <memory>
#include <new> // std::hardware_destructive_interference_size
#include <stdexcept>

#ifndef __cpp_aligned_new
#include <stdlib.h> // posix_memalign
#endif

#if __has_cpp_attribute(nodiscard)
#define NODISCARD [[nodiscard]]
#endif
#ifndef NODISCARD
#define NODISCARD
#endif

namespace mpmc
{
#if defined(__cpp_lib_hardware_interference_size) && !defined(__APPLE__)
  static constexpr size_t hardwareInterferenceSize =
      std::hardware_destructive_interference_size;
#else
  /* 缓存行对齐 */
  static constexpr size_t hardwareInterferenceSize = 64;
#endif

#if defined(__cpp_aligned_new)
  template <typename T>
  using AlignedAllocator = std::allocator<T>;
#else
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
#endif

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
      // static_assert(std::is_nothrow_constructible<T, Args &&...>::value,
      //               "T must be nothrow constructible with Args&&...");
      new (&storage) T(std::forward<Args>(args)...);
    }

    void destroy() noexcept
    {
      static_assert(std::is_nothrow_destructible<T>::value,
                    "T must be nothrow destructible");
      reinterpret_cast<T *>(&storage)->~T();
    }

    T &&move() noexcept { return reinterpret_cast<T &&>(storage); }

    // Align to avoid false sharing between adjacent slots
    alignas(hardwareInterferenceSize) std::atomic<size_t> turn = {0}; // alignas(64) c++11
    typename std::aligned_storage<sizeof(T), alignof(T)>::type storage;
  };

  template <typename T, typename Allocator = AlignedAllocator<Slot<T>>>
  class Queue
  {
  private:
    static_assert(std::is_nothrow_copy_assignable<T>::value ||
                      std::is_nothrow_move_assignable<T>::value,
                  "T must be nothrow copy or move assignable");

    static_assert(std::is_nothrow_destructible<T>::value,
                  "T must be nothrow destructible");

  public:
    explicit Queue(const size_t capacity,
                   const Allocator &allocator = Allocator())
        : capacity_(capacity), allocator_(allocator), head_(0), tail_(0)
    {
      if (capacity_ < 1)
      {
        throw std::invalid_argument("capacity < 1");
      }
      // Allocate one extra slot to prevent false sharing on the last slot
      slots_ = allocator_.allocate(capacity_ + 1);
      // Allocators are not required to honor alignment for over-aligned types
      // (see http://eel.is/c++draft/allocator.requirements#10) so we verify
      // alignment here
      if (reinterpret_cast<size_t>(slots_) % alignof(Slot<T>) != 0)
      {
        allocator_.deallocate(slots_, capacity_ + 1);
        throw std::bad_alloc();
      }
      for (size_t i = 0; i < capacity_; ++i)
      {
        new (&slots_[i]) Slot<T>();
      }
      static_assert(
          alignof(Slot<T>) == hardwareInterferenceSize,
          "Slot must be aligned to cache line boundary to prevent false sharing");
      static_assert(sizeof(Slot<T>) % hardwareInterferenceSize == 0,
                    "Slot size must be a multiple of cache line size to prevent "
                    "false sharing between adjacent slots");
      static_assert(sizeof(Queue) % hardwareInterferenceSize == 0,
                    "Queue size must be a multiple of cache line size to "
                    "prevent false sharing between adjacent queues");
      static_assert(
          offsetof(Queue, tail_) - offsetof(Queue, head_) ==
              static_cast<std::ptrdiff_t>(hardwareInterferenceSize),
          "head and tail must be a cache line apart to prevent false sharing");
    }

    ~Queue() noexcept
    {
      for (size_t i = 0; i < capacity_; ++i)
      {
        slots_[i].~Slot();
      }
      allocator_.deallocate(slots_, capacity_ + 1);
    }

    // non-copyable and non-movable
    Queue(const Queue &) = delete;
    Queue &operator=(const Queue &) = delete;

    template <typename... Args>
    void emplace(Args &&...args) noexcept
    {
      // static_assert(std::is_nothrow_constructible<T, Args &&...>::value,
      //               "T must be nothrow constructible with Args&&...");
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
      // static_assert(std::is_nothrow_constructible<T, Args &&...>::value,
      //               "T must be nothrow constructible with Args&&...");
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
      static_assert(std::is_nothrow_copy_constructible<T>::value,
                    "T must be nothrow copy constructible");
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
      static_assert(std::is_nothrow_copy_constructible<T>::value,
                    "T must be nothrow copy constructible");
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

    /// Returns the number of elements in the queue.
    /// The size can be negative when the queue is empty and there is at least one
    /// reader waiting. Since this is a concurrent queue the size is only a best
    /// effort guess until all reader and writer threads have been joined.
    ptrdiff_t size() const noexcept
    {
      // TODO: How can we deal with wrapped queue on 32bit?
      return static_cast<ptrdiff_t>(head_.load(std::memory_order_relaxed) -
                                    tail_.load(std::memory_order_relaxed));
    }

    /// Returns true if the queue is empty.
    /// Since this is a concurrent queue this is only a best effort guess
    /// until all reader and writer threads have been joined.
    bool empty() const noexcept { return size() <= 0; }

  private:
    constexpr size_t idx(size_t i) const noexcept { return i % capacity_; }

    constexpr size_t turn(size_t i) const noexcept { return i / capacity_; }

  private:
    const size_t capacity_;
    Slot<T> *slots_;
#if defined(__has_cpp_attribute) && __has_cpp_attribute(no_unique_address)
    Allocator allocator_ [[no_unique_address]];
#else
    Allocator allocator_;
#endif

    // Align to avoid false sharing between head_ and tail_
    alignas(hardwareInterferenceSize) std::atomic<size_t> head_;
    alignas(hardwareInterferenceSize) std::atomic<size_t> tail_;
  };
} // namespace mpmc

template <typename T, typename Allocator = std::allocator<T>> class SPSCQueue {

#if defined(__cpp_if_constexpr) && defined(__cpp_lib_void_t)
  template <typename Alloc2, typename = void>
  struct has_allocate_at_least : std::false_type {};

  template <typename Alloc2>
  struct has_allocate_at_least<
      Alloc2, std::void_t<typename Alloc2::value_type,
                          decltype(std::declval<Alloc2 &>().allocate_at_least(
                              size_t{}))>> : std::true_type {};
#endif

public:
  explicit SPSCQueue(const size_t capacity,
                     const Allocator &allocator = Allocator())
      : capacity_(capacity), allocator_(allocator) {
    // The queue needs at least one element
    if (capacity_ < 1) {
      capacity_ = 1;
    }
    capacity_++; // Needs one slack element
    // Prevent overflowing size_t
    if (capacity_ > SIZE_MAX - 2 * kPadding) {
      capacity_ = SIZE_MAX - 2 * kPadding;
    }

#if defined(__cpp_if_constexpr) && defined(__cpp_lib_void_t)
    if constexpr (has_allocate_at_least<Allocator>::value) {
      auto res = allocator_.allocate_at_least(capacity_ + 2 * kPadding);
      slots_ = res.ptr;
      capacity_ = res.count - 2 * kPadding;
    } else {
      slots_ = std::allocator_traits<Allocator>::allocate(
          allocator_, capacity_ + 2 * kPadding);
    }
#else
    slots_ = std::allocator_traits<Allocator>::allocate(
        allocator_, capacity_ + 2 * kPadding);
#endif

    static_assert(alignof(SPSCQueue<T>) == kCacheLineSize, "");
    static_assert(sizeof(SPSCQueue<T>) >= 3 * kCacheLineSize, "");
    assert(reinterpret_cast<char *>(&readIdx_) -
               reinterpret_cast<char *>(&writeIdx_) >=
           static_cast<std::ptrdiff_t>(kCacheLineSize));
  }

  ~SPSCQueue() {
    while (front()) {
      pop();
    }
    std::allocator_traits<Allocator>::deallocate(allocator_, slots_,
                                                 capacity_ + 2 * kPadding);
  }

  // non-copyable and non-movable
  SPSCQueue(const SPSCQueue &) = delete;
  SPSCQueue &operator=(const SPSCQueue &) = delete;

  template <typename... Args>
  void emplace(Args &&...args) noexcept(
      std::is_nothrow_constructible<T, Args &&...>::value) {
    static_assert(std::is_constructible<T, Args &&...>::value,
                  "T must be constructible with Args&&...");
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
  NODISCARD bool try_emplace(Args &&...args) noexcept(
      std::is_nothrow_constructible<T, Args &&...>::value) {
    static_assert(std::is_constructible<T, Args &&...>::value,
                  "T must be constructible with Args&&...");
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

  void push(const T &v) noexcept(std::is_nothrow_copy_constructible<T>::value) {
    static_assert(std::is_copy_constructible<T>::value,
                  "T must be copy constructible");
    emplace(v);
  }

  template <typename P, typename = typename std::enable_if<
                            std::is_constructible<T, P &&>::value>::type>
  void push(P &&v) noexcept(std::is_nothrow_constructible<T, P &&>::value) {
    emplace(std::forward<P>(v));
  }

  NODISCARD bool
  try_push(const T &v) noexcept(std::is_nothrow_copy_constructible<T>::value) {
    static_assert(std::is_copy_constructible<T>::value,
                  "T must be copy constructible");
    return try_emplace(v);
  }

  template <typename P, typename = typename std::enable_if<
                            std::is_constructible<T, P &&>::value>::type>
  NODISCARD bool
  try_push(P &&v) noexcept(std::is_nothrow_constructible<T, P &&>::value) {
    return try_emplace(std::forward<P>(v));
  }

  NODISCARD T *front() noexcept {
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
    static_assert(std::is_nothrow_destructible<T>::value,
                  "T must be nothrow destructible");
    auto const readIdx = readIdx_.load(std::memory_order_relaxed);
    assert(writeIdx_.load(std::memory_order_acquire) != readIdx);
    slots_[readIdx + kPadding].~T();
    auto nextReadIdx = readIdx + 1;
    if (nextReadIdx == capacity_) {
      nextReadIdx = 0;
    }
    readIdx_.store(nextReadIdx, std::memory_order_release);
  }

  NODISCARD size_t size() const noexcept {
    std::ptrdiff_t diff = writeIdx_.load(std::memory_order_acquire) -
                          readIdx_.load(std::memory_order_acquire);
    if (diff < 0) {
      diff += capacity_;
    }
    return static_cast<size_t>(diff);
  }

  NODISCARD bool empty() const noexcept {
    return writeIdx_.load(std::memory_order_acquire) ==
           readIdx_.load(std::memory_order_acquire);
  }

  NODISCARD size_t capacity() const noexcept { return capacity_ - 1; }

private:
#ifdef __cpp_lib_hardware_interference_size
  static constexpr size_t kCacheLineSize =
      std::hardware_destructive_interference_size;
#else
  static constexpr size_t kCacheLineSize = 64;
#endif

  // Padding to avoid false sharing between slots_ and adjacent allocations
  static constexpr size_t kPadding = (kCacheLineSize - 1) / sizeof(T) + 1;

private:
  size_t capacity_;
  T *slots_;
#if defined(__has_cpp_attribute) && __has_cpp_attribute(no_unique_address)
  Allocator allocator_ [[no_unique_address]];
#else
  Allocator allocator_;
#endif

  // Align to cache line size in order to avoid false sharing
  // readIdxCache_ and writeIdxCache_ is used to reduce the amount of cache
  // coherency traffic
  alignas(kCacheLineSize) std::atomic<size_t> writeIdx_ = {0};
  alignas(kCacheLineSize) size_t readIdxCache_ = 0;
  alignas(kCacheLineSize) std::atomic<size_t> readIdx_ = {0};
  alignas(kCacheLineSize) size_t writeIdxCache_ = 0;

  // Padding to avoid adjacent allocations to share cache line with
  // writeIdxCache_
  char padding_[kCacheLineSize - sizeof(writeIdxCache_)];
};

template <typename T,
          typename Allocator = mpmc::AlignedAllocator<mpmc::Slot<T>>>
using MPMCQueue = mpmc::Queue<T, Allocator>;