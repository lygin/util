#include <gtest/gtest.h>
#define XXH_INLINE_ALL
#include "xxhash.h"
#include "murmur2.h"
#include "murmur3.h"

constexpr int TEST_NUM = 10'000'000;
TEST(xxhash, _32) {
  for(int i=0; i<TEST_NUM; i++) {
    volatile uint32_t hash = XXH32(&i, 4, 5381);
  }
}

TEST(xxhash, _64) {
  for(int i=0; i<TEST_NUM; i++) {
    volatile uint64_t hash = XXH64(&i, 4, 5381);
  }
}

TEST(xxhash, _3_64) {
  for(int i=0; i<TEST_NUM; i++) {
    volatile uint64_t hash = XXH3_64bits(&i, 4);
  }
}

TEST(xxhash, _3_128) {
  for(int i=0; i<TEST_NUM; i++) {
    volatile XXH128_hash_t hash = XXH3_128bits(&i, 4);
  }
}

TEST(murmur2, _32) {
  for(int i=0; i<TEST_NUM; i++) {
    volatile uint32_t hash = Hash(&i, 4, 5381);
  }
}

TEST(murmur3, _32) {
  for(int i=0; i<TEST_NUM; i++) {
    volatile uint32_t hash = MurmurHash3_x86_32(&i, 4, 5381);
  }
}

/**
 * xxhash._32 (20 ms)
 * xxhash._64 (22 ms)
 * xxhash._3_64 (20 ms)
 * xxhash._3_128 (21 ms)
 * murmur2._32 (10 ms)
 * murmur3._32 (20 ms)
*/