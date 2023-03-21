#include <gtest/gtest.h>
#include "bloomfilter.h"
#include "bloom.h"
#include <memory>
using namespace std;


TEST(bloomfilter, basic) {
  string filter;
  shared_ptr<BloomFilter> bf = make_shared<BloomFilter>(10);
  vector<string> keys;
  for (int i = 0; i < 26; ++i) {
    char c = i + 'A';
    string x = "xx" + string(1, c);
    keys.emplace_back(x);
  }

  vector<Slice> key_slices;
  for (auto &&key : keys) {
    key_slices.emplace_back(key);
  }
  bf->CreateFilter(&key_slices[0], key_slices.size(), &filter);
  keys.clear();
  key_slices.clear();
  ASSERT_EQ(filter.size(), (26 * 10 + 7) / 8 + 1);

  EXPECT_EQ(bf->KeyMayMatch("xxA", filter), true);
  EXPECT_EQ(bf->KeyMayMatch("xxZ", filter), true);
  EXPECT_EQ(bf->KeyMayMatch("xxa", filter), false);
  EXPECT_EQ(bf->KeyMayMatch("xxz", filter), false);
}

TEST(bloom, basic) {
  Bloom bf;
  bf.Init(1000, 0.01);
  for(int i=0; i<100; ++i) {
    EXPECT_EQ(bf.Add(&i, 4), BloomStatus::OK);
  }
  for(int i=0; i<100; ++i) {
    EXPECT_EQ(bf.Check(&i, 4), BloomStatus::kMayExist);
  }
  for(int i=100; i<1000; ++i) {
    EXPECT_EQ(bf.Check(&i, 4), BloomStatus::kNotExist);
  }
}