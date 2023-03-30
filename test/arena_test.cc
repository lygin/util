#include "arena.h"
#include "Random.h"
#include <gtest/gtest.h>

TEST(ArenaTest, Empty) { Arena arena; }

TEST(ArenaTest, Simple) {
  std::vector<std::pair<size_t, char*>> allocated;
  Arena arena;
  const int N = 100000;
  size_t bytes = 0;
  Random rnd;
  for (int i = 0; i < N; i++) {
    size_t s;
    if (i % (N / 10) == 0) {
      s = i;
    } else {
      s = rnd.OneIn(4000)
              ? rnd.Uniform(6000)
              : (rnd.OneIn(10) ? rnd.Uniform(100) : rnd.Uniform(20));
    }
    if (s == 0) {
      // Our arena disallows size 0 allocations.
      s = 1;
    }
    char* r;
    if (rnd.OneIn(10)) {
      r = arena.AllocateAligned(s);
    } else {
      r = arena.Allocate(s);
    }

    for (size_t b = 0; b < s; b++) {
      // Fill the "i"th allocation with a known bit pattern
      r[b] = i % 256;
    }
    bytes += s;
    allocated.push_back(std::make_pair(s, r));
    ASSERT_GE(arena.MemoryUsage(), bytes);
    if (i > N / 10) {
      ASSERT_LE(arena.MemoryUsage(), bytes * 1.10);
    }
  }
  for (size_t i = 0; i < allocated.size(); i++) {
    size_t num_bytes = allocated[i].first;
    const char* p = allocated[i].second;
    for (size_t b = 0; b < num_bytes; b++) {
      // Check the "i"th allocation for the known bit pattern
      ASSERT_EQ(int(p[b]) & 0xff, i % 256);
    }
  }
}

TEST(FixedArenaTest, AllocateAndFreeFixed) {
  const int32_t item_size = 16;
  const int32_t num_items = 10;
  FixedArena arena(item_size);

  // Allocate fixed-size chunks of memory
  char *ptrs[num_items];
  for (int i = 0; i < num_items; ++i) {
    ptrs[i] = arena.AllocateFixed(item_size);
    EXPECT_TRUE(ptrs[i] != nullptr);
  }

  // Try to allocate a chunk of memory that is too large
  // char *ptr = arena.AllocateFixed(item_size + 1);
  // EXPECT_TRUE(ptr == nullptr);

  // Free some of the fixed-size chunks of memory
  arena.FreeFixed(ptrs[0]);
  arena.FreeFixed(ptrs[2]);
  arena.FreeFixed(ptrs[4]);

  // Allocate more fixed-size chunks of memory
  char *ptrs2[3];
  for (int i = 0; i < 3; ++i) {
    ptrs2[i] = arena.AllocateFixed(item_size);
    EXPECT_TRUE(ptrs2[i] != nullptr);
  }

  // Check that the same chunks of memory were reused
  EXPECT_EQ(ptrs[0], ptrs2[0]);
  EXPECT_EQ(ptrs[2], ptrs2[1]);
  EXPECT_EQ(ptrs[4], ptrs2[2]);
}