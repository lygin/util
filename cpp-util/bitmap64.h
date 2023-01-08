#ifndef _BITMAP64_H_
#define _BITMAP64_H_
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>

class Bitmap64 {
public:
  Bitmap64(uint32_t bits) {
    bits_ = bits;
    n64_ = (bits + 63) / 64;
    int rest = bits % 64;
    printf("%d\n", rest);
    data_ = (uint64_t*)calloc(n64_, sizeof(uint64_t));
    if(rest != 0) {
      data_[n64_-1] = ~0 ^ ((1ull<<rest) - 1);
    }
  }
  ~Bitmap64() {
    free(data_);
  }

  bool Full() const {
    for (uint32_t i = 0; i < n64_; i++) {
      if (~data_[i]) {
        return false;
      }
    }
    return true;
  }

  // return the first free position of bitmap, return -1 if full
  int FirstFreePos() const {
    for (uint32_t i = 0; i < n64_; i++) {
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
  uint32_t bits_; // total bits used
  uint32_t n64_;    // total uint64
  uint64_t *data_;
};

#endif