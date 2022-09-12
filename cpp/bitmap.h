#ifndef __BITMAP_H__
#define __BITMAP_H__
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>

constexpr static uint64_t MASK[] = {
    0,
    0x1ULL,
    0x3ULL,
    0x7ULL,
    0xfULL,
    0x1fULL,
    0x3fULL,
    0x7fULL,
    0xffULL,
    0x1ffULL,
    0x3ffULL,
    0x7ffULL,
    0xfffULL,
    0x1fffULL,
    0x3fffULL,
    0x7fffULL,
    0xffffULL,
    0x1ffffULL,
    0x3ffffULL,
    0x7ffffULL,
    0xfffffULL,
    0x1fffffULL,
    0x3fffffULL,
    0x7fffffULL,
    0xffffffULL,
    0x1ffffffULL,
    0x3ffffffULL,
    0x7ffffffULL,
    0xfffffffULL,
    0x1fffffffULL,
    0x3fffffffULL,
    0x7fffffffULL,
    0xffffffffULL,
    0x1ffffffffULL,
    0x3ffffffffULL,
    0x7ffffffffULL,
    0xfffffffffULL,
    0x1fffffffffULL,
    0x3fffffffffULL,
    0x7fffffffffULL,
    0xffffffffffULL,
    0x1ffffffffffULL,
    0x3ffffffffffULL,
    0x7ffffffffffULL,
    0xfffffffffffULL,
    0x1fffffffffffULL,
    0x3fffffffffffULL,
    0x7fffffffffffULL,
    0xffffffffffffULL,
    0x1ffffffffffffULL,
    0x3ffffffffffffULL,
    0x7ffffffffffffULL,
    0xfffffffffffffULL,
    0x1fffffffffffffULL,
    0x3fffffffffffffULL,
    0x7fffffffffffffULL,
    0xffffffffffffffULL,
    0x1ffffffffffffffULL,
    0x3ffffffffffffffULL,
    0x7ffffffffffffffULL,
    0xfffffffffffffffULL,
    0x1fffffffffffffffULL,
    0x3fffffffffffffffULL,
    0x7fffffffffffffffULL,
};

class Bitmap {
public:
  Bitmap() = delete;
  bool Full() const {
    for (uint32_t i = 0; i < n_; i++) {
      if (~data_[i]) {
        return false;
      }
    }
    return true;
  }

  // return the first free position of bitmap, return -1 if full
  int FirstFreePos() const {
    for (uint32_t i = 0; i < n_; i++) {
      // find first zero in data_[i]
      int ffp = ffsl(~data_[i]) - 1;
      if (ffp != -1) {
        return i * 64 + ffp;
      }
    }
    return -1;
  }

  bool Test(int index) const {
    assert(index < bits_);
    int n = index / 64;
    int off = index % 64;
    return data_[n] & (1ULL << off);
  }

  void Reset(int index) {
    assert(index < bits_);
    int n = index / 64;
    int off = index % 64;
    data_[n] &= ~(1ULL << off);
  }
  // set idx
  bool Set(int index) {
    assert(index < bits_);
    int n = index / 64;
    int off = index % 64;
    if (data_[n] & (1ULL << off)) {
      return false;
    }
    data_[n] |= (1ULL << off);
    return true;
  }

private:
  friend Bitmap *NewBitmap(uint32_t size);
  uint32_t bits_; // total bits used
  uint32_t n_;    // total uint64
  uint64_t data_[0];
};

__always_inline Bitmap *NewBitmap(uint32_t bits) {
  static_assert(sizeof(uint64_t) == 8, "sizeof(uint64_t) != 8!!!");
  uint32_t n = (bits + 63) / 64;
  int tmp = bits % 64;
  Bitmap *bitmap =
      reinterpret_cast<Bitmap *>(malloc(sizeof(Bitmap) + n * sizeof(uint64_t)));
  bitmap->bits_ = bits;
  bitmap->n_ = n;
  memset((char *)bitmap->data_, 0, n * sizeof(uint64_t));
  if (tmp != 0) {
    bitmap->data_[n - 1] |= (MASK[64 - tmp] << tmp);
  }
  return bitmap;
}

__always_inline void DeleteBitmap(Bitmap *bitmap) { free(bitmap); }

#endif