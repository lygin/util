#ifndef _COMMOM_H_
#define _COMMOM_H_

#include <stdexcept>

#define likely(x) __builtin_expect(!!(x), 1) 
#define unlikely(x) __builtin_expect(!!(x), 0)

#define always_inline inline __attribute__((always_inline))
#define packed __attribute__((__packed__))
#define aligned(n) __attribute__ ((aligned(n)))

static always_inline uint64_t align(uint64_t x, uint64_t base) {
  return (((x) + (base)-1) & ~(base - 1));
}

static always_inline uint32_t alignUpPowerOf2(uint32_t x) {
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return x + 1;
}

static always_inline uint64_t alignUpPowerOf2(uint64_t x) {
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x |= x >> 32;
  return x + 1;
}

#endif