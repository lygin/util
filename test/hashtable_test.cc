#include <gtest/gtest.h>
#include <string>
#include <unordered_map>
#include <cstdio>
#include <random>
#include "ska_hashtable.h"
#include "unordered_dense_map.h"
#include "Random.h"
#include "timer.h"

const int N = 10'0000;

std::vector<uint64_t> test_keys(N);
std::vector<std::string> test_skeys(N);
std::string dataset = "../dataset/dataset.txt";

void clear_cache()
{
#ifdef CLEAR_CACHE
  int *dummy = new int[1024 * 1024 * 256];
  for (int i = 0; i < 1024 * 1024 * 256; i++)
  {
    dummy[i] = i;
  }

  for (int i = 100; i < 1024 * 1024 * 256 - 100; i++)
  {
    dummy[i] = dummy[i - rand() % 100] + dummy[i + rand() % 100];
  }

  delete[] dummy;
#endif
}

TEST(INIT, INIT)
{
  FILE *fp = fopen(dataset.c_str(), "r");
  if (fp == NULL)
  {
    std::cerr << "ERROR: Couldn't open\n";
    return;
  }
  for (int i = 0; i < N; i++)
  {
    fscanf(fp, "%lu", &test_keys[i]);
    test_skeys[i] = std::to_string(test_keys[i]);
  }
  printf("reading complete\n");
}

TEST(ska_hashtable_test, st)
{
  /* init table */
  ska::flat_hash_map<uint64_t, uint64_t> ska_table;
  clear_cache();
  Timer t;
  for (int i = 0; i < N; ++i)
  {
    ska_table.emplace(test_keys[i], test_keys[i]);
  }
  double elapsed = t.GetDurationNs();
  printf("Insertion elapsed: %.3f us %f Mops\n", elapsed/1000, (double)N/(elapsed/1000.0));
  
  clear_cache();
  t.Reset();
  for(int i=0; i<N; ++i) {
    auto ret = ska_table.at(test_keys[i]);
    ASSERT_EQ(ret, test_keys[i]);
  }
  elapsed = t.GetDurationNs();
  printf("Search elapsed: %.3f us %f Mops\n", elapsed/1000, (double)N/(elapsed/1000.0));
}

TEST(stl_hashtable_test, st)
{
  /* init table */
  std::unordered_map<uint64_t, uint64_t> std_table;
  clear_cache();
  Timer t;
  for (int i = 0; i < N; ++i)
  {
    std_table.emplace(test_keys[i], test_keys[i]);
  }
  double elapsed = t.GetDurationNs();
  printf("Insertion elapsed: %.3f us %f Mops\n", elapsed/1000, (double)N/(elapsed/1000.0));
  
  clear_cache();
  t.Reset();
  for(int i=0; i<N; ++i) {
    auto ret = std_table.at(test_keys[i]);
    ASSERT_EQ(ret, test_keys[i]);
  }
  elapsed = t.GetDurationNs();
  printf("Search elapsed: %.3f us %f Mops\n", elapsed/1000, (double)N/(elapsed/1000.0));
}

TEST(unordered_dense, st)
{
  /* init table */
  ankerl::unordered_dense::map<uint64_t, uint64_t> ank_table;
  clear_cache();
  Timer t;
  for (int i = 0; i < N; ++i)
  {
    ank_table.emplace(test_keys[i], test_keys[i]);
  }
  double elapsed = t.GetDurationNs();
  printf("Insertion elapsed: %.3f us %f Mops\n", elapsed/1000, (double)N/(elapsed/1000.0));
  
  clear_cache();
  t.Reset();
  for(int i=0; i<N; ++i) {
    auto ret = ank_table.at(test_keys[i]);
    ASSERT_EQ(ret, test_keys[i]);
  }
  elapsed = t.GetDurationNs();
  printf("Search elapsed: %.3f us %f Mops\n", elapsed/1000, (double)N/(elapsed/1000.0));
}

TEST(ska_hashtable_test, st_string)
{
  /* init table */
  ska::flat_hash_map<std::string, std::string> ska_table;
  clear_cache();
  Timer t;
  for (int i = 0; i < N; ++i)
  {
    ska_table.emplace(test_skeys[i], test_skeys[i]);
  }
  double elapsed = t.GetDurationNs();
  printf("Insertion elapsed: %.3f us %f Mops\n", elapsed/1000, (double)N/(elapsed/1000.0));
  
  clear_cache();
  t.Reset();
  for(int i=0; i<N; ++i) {
    auto ret = ska_table.at(test_skeys[i]);
    ASSERT_EQ(ret, test_skeys[i]);
  }
  elapsed = t.GetDurationNs();
  printf("Search elapsed: %.3f us %f Mops\n", elapsed/1000, (double)N/(elapsed/1000.0));
}

TEST(stl_hashtable_test, st_string)
{
  /* init table */
  std::unordered_map<std::string, std::string> ska_table;
  clear_cache();
  Timer t;
  for (int i = 0; i < N; ++i)
  {
    ska_table.emplace(test_skeys[i], test_skeys[i]);
  }
  double elapsed = t.GetDurationNs();
  printf("Insertion elapsed: %.3f us %f Mops\n", elapsed/1000, (double)N/(elapsed/1000.0));
  
  clear_cache();
  t.Reset();
  for(int i=0; i<N; ++i) {
    auto ret = ska_table.at(test_skeys[i]);
    ASSERT_EQ(ret, test_skeys[i]);
  }
  elapsed = t.GetDurationNs();
  printf("Search elapsed: %.3f us %f Mops\n", elapsed/1000, (double)N/(elapsed/1000.0));
}

TEST(unordered_dense, st_string)
{
  /* init table */
  ankerl::unordered_dense::map<std::string, std::string> ska_table;
  clear_cache();
  Timer t;
  for (int i = 0; i < N; ++i)
  {
    ska_table.emplace(test_skeys[i], test_skeys[i]);
  }
  double elapsed = t.GetDurationNs();
  printf("Insertion elapsed: %.3f us %f Mops\n", elapsed/1000, (double)N/(elapsed/1000.0));
  
  clear_cache();
  t.Reset();
  for(int i=0; i<N; ++i) {
    auto ret = ska_table.at(test_skeys[i]);
    ASSERT_EQ(ret, test_skeys[i]);
  }
  elapsed = t.GetDurationNs();
  printf("Search elapsed: %.3f us %f Mops\n", elapsed/1000, (double)N/(elapsed/1000.0));
}

/**
 * 结果： 
 * ref : https://martin.ankerl.com/2022/08/27/hashmap-bench-01/
 *         int64 string memoryUsage
 * ankerl   130   117    199  (C++17) 
 * ska      120   183    450 
 * stl      345   286    370 
 * --------real(ms)-------real(MB)-----------
 *         int64 string memoryUsage
 * ankerl   37   72     94\100 (C++17)
 * ska      34   86     89\107
 * stl      58   87     89\94
 * 总结：
 * for int64: ska or ankerl
 * for string: ankerl
 */
