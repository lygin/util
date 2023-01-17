/**
 * no thread safty, used for thread local
 * no free function for performance, when arena destructor is called, all memory is freed
*/
#ifndef _ARENA_H_
#define _ARENA_H_

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

class Allocator {
 public:
  virtual ~Allocator() {}

  virtual char* Allocate(size_t bytes) = 0;
  virtual char* AllocateAligned(size_t bytes) = 0;
};

class Arena : public Allocator {
  static const int kBlockSize = 4096;
 public:
  Arena() : alloc_ptr_(nullptr), alloc_bytes_remaining_(0), memory_usage_(0) {}

  Arena(const Arena&) = delete;
  Arena& operator=(const Arena&) = delete;

  ~Arena() {
    for (size_t i = 0; i < blocks_.size(); i++) {
      free(blocks_[i]);
    }
  }

  char* Allocate(size_t bytes) {
    assert(bytes > 0);
    if (bytes < alloc_bytes_remaining_) {
      char *ptr = alloc_ptr_;
      alloc_ptr_ += bytes;
      alloc_bytes_remaining_ -= bytes;
      return ptr;
    }
    return AllocateFallback(bytes);
  }
  // alignas(8)
  char* AllocateAligned(size_t bytes) {
    const int align = sizeof(void*);
    size_t mod = reinterpret_cast<uintptr_t>(alloc_ptr_) & (align - 1);
    size_t padding = (mod == 0 ? 0 : align - mod);
    // ptr|padding|ptr(aligned 8)
    size_t needed = padding + bytes;
    char* ptr;
    if (needed <= alloc_bytes_remaining_) {
      ptr = alloc_ptr_ + padding;
      alloc_ptr_ += needed;
      alloc_bytes_remaining_ -= needed;
    } else {
      ptr = AllocateFallback(bytes);
    }
    // check if ptr is aligned
    assert((reinterpret_cast<uintptr_t>(ptr) & (align - 1)) == 0);
    return ptr;
  }

  size_t MemoryUsage() const {
    return memory_usage_.load(std::memory_order_relaxed);
  }

 private:
  char* AllocateFallback(size_t bytes) {
    // Object is more than a quarter of our block size.  Allocate it separately
    // to avoid wasting too much space in current block leftover bytes.
    if (bytes > kBlockSize / 4) {
      char* result = AllocateNewBlock(bytes);
      return result;
    }
    // We waste the remaining space in the current block.
    alloc_ptr_ = AllocateNewBlock(kBlockSize);
    alloc_bytes_remaining_ = kBlockSize;

    char* ptr = alloc_ptr_;
    alloc_ptr_ += bytes;
    alloc_bytes_remaining_ -= bytes;
    return ptr;
  }
  char* AllocateNewBlock(size_t block_bytes) {
    char* ptr = (char*)malloc(block_bytes);
    assert(ptr != nullptr);
    blocks_.push_back(ptr);
    // ptr + block
    memory_usage_.fetch_add(block_bytes + sizeof(char*),std::memory_order_relaxed);
    return ptr;
  }

  // 最新block可分配的开始地址
  char* alloc_ptr_;
  // 最新block剩余可分配的bytes
  size_t alloc_bytes_remaining_;
  // 已经分配的blcok地址
  std::vector<char*> blocks_;
  // Total memory usage of the arena.
  std::atomic<size_t> memory_usage_;
};

#endif  // _ARENA_H_
