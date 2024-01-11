#include <gtest/gtest.h>
#include <string>
#include <unordered_map>
#include <cstdio>
#include <random>
#include <thread>
#include "tbb/concurrent_hash_map.h"
#include "ska_hashtable.h"
#include "unordered_dense_map.h"
#include "cas_hashtable.h"
#include "Random.h"
#include "timer.h"
#include "cuckoomap/cuckoohash_map.hh"
using namespace std;

const int N = 200'000;
static const int numThreads = 8;

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

TEST(ska_hashtable, st_int64)
{
  /* init table */
  ska::flat_hash_map<uint64_t, uint64_t> ska_table;
  ska_table.reserve(N);
  clear_cache();
  Timer t;
  for (int i = 0; i < N; ++i)
  {
    ska_table.emplace(test_keys[i], test_keys[i]);
  }
  double elapsed = t.GetDurationNs();
  printf("Insertion %f Mops\n",(double)N/(elapsed/1000.0));
  
  clear_cache();
  t.Reset();
  for(int i=0; i<N; ++i) {
    auto ret = ska_table.at(test_keys[i]);
    ASSERT_EQ(ret, test_keys[i]);
  }
  elapsed = t.GetDurationNs();
  printf("Search %f Mops\n", (double)N/(elapsed/1000.0));
}

TEST(stl_hashtable, st_int64)
{
  /* init table */
  std::unordered_map<uint64_t, uint64_t> std_table;
  std_table.reserve(N);
  clear_cache();
  Timer t;
  for (int i = 0; i < N; ++i)
  {
    std_table.emplace(test_keys[i], test_keys[i]);
  }
  double elapsed = t.GetDurationNs();
  printf("Insertion %f Mops\n", (double)N/(elapsed/1000.0));
  
  clear_cache();
  t.Reset();
  for(int i=0; i<N; ++i) {
    auto ret = std_table.at(test_keys[i]);
    ASSERT_EQ(ret, test_keys[i]);
  }
  elapsed = t.GetDurationNs();
  printf("Search %f Mops\n",(double)N/(elapsed/1000.0));
}

TEST(unordered_dense, st_int64)
{
  /* init table */
  ankerl::unordered_dense::map<uint64_t, uint64_t> ank_table;
  ank_table.reserve(N);
  clear_cache();
  Timer t;
  for (int i = 0; i < N; ++i)
  {
    ank_table.emplace(test_keys[i], test_keys[i]);
  }
  double elapsed = t.GetDurationNs();
  printf("Insertion %f Mops\n",(double)N/(elapsed/1000.0));
  
  clear_cache();
  t.Reset();
  for(int i=0; i<N; ++i) {
    auto ret = ank_table.at(test_keys[i]);
    ASSERT_EQ(ret, test_keys[i]);
  }
  elapsed = t.GetDurationNs();
  printf("Search %f Mops\n", (double)N/(elapsed/1000.0));
}

TEST(ska_hashtable, st_string)
{
  /* init table */
  ska::flat_hash_map<std::string, std::string> ska_table;
  ska_table.reserve(N);
  clear_cache();
  Timer t;
  for (int i = 0; i < N; ++i)
  {
    ska_table.emplace(test_skeys[i], test_skeys[i]);
  }
  double elapsed = t.GetDurationNs();
  printf("Insertion %f Mops\n", (double)N/(elapsed/1000.0));
  
  clear_cache();
  t.Reset();
  for(int i=0; i<N; ++i) {
    auto ret = ska_table.at(test_skeys[i]);
    ASSERT_EQ(ret, test_skeys[i]);
  }
  elapsed = t.GetDurationNs();
  printf("Search %f Mops\n", (double)N/(elapsed/1000.0));
}

TEST(stl_hashtable, st_string)
{
  /* init table */
  std::unordered_map<std::string, std::string> ska_table;
  ska_table.reserve(N);
  clear_cache();
  Timer t;
  for (int i = 0; i < N; ++i)
  {
    ska_table.emplace(test_skeys[i], test_skeys[i]);
  }
  double elapsed = t.GetDurationNs();
  printf("Insertion %f Mops\n",(double)N/(elapsed/1000.0));
  
  clear_cache();
  t.Reset();
  for(int i=0; i<N; ++i) {
    auto ret = ska_table.at(test_skeys[i]);
    ASSERT_EQ(ret, test_skeys[i]);
  }
  elapsed = t.GetDurationNs();
  printf("Search %f Mops\n", (double)N/(elapsed/1000.0));
}

TEST(unordered_dense, st_string)
{
  /* init table */
  ankerl::unordered_dense::map<std::string, std::string> ska_table;
  ska_table.reserve(N);
  clear_cache();
  Timer t;
  for (int i = 0; i < N; ++i)
  {
    ska_table.emplace(test_skeys[i], test_skeys[i]);
  }
  double elapsed = t.GetDurationNs();
  printf("Insertion %f Mops\n", (double)N/(elapsed/1000.0));
  
  clear_cache();
  t.Reset();
  for(int i=0; i<N; ++i) {
    auto ret = ska_table.at(test_skeys[i]);
    ASSERT_EQ(ret, test_skeys[i]);
  }
  elapsed = t.GetDurationNs();
  printf("Search %f Mops\n", (double)N/(elapsed/1000.0));
}

TEST(tbb_conhashtable, mt_int64_loadc) {
  using namespace tbb;
  using namespace std;
  concurrent_hash_map<uint64_t, uint64_t> ht(65536);
  vector<thread> insertingThreads;
  vector<thread> searchingThreads;
  vector<int> failed(numThreads);
  auto insert = [&ht](int from, int to) {
	  for(int i=from; i<to; i++) {
      concurrent_hash_map<uint64_t, uint64_t>::accessor accessor;
	    ht.insert(accessor, test_keys[i]);
      accessor->second = test_keys[i];
	  }
  };
  auto search = [&ht, &failed](int from, int to, int tid){
	  int fail = 0;
	  for(int i=from; i<to; i++){
      concurrent_hash_map<uint64_t, uint64_t>::const_accessor ca;
	    bool ret = ht.find(ca, test_keys[i]);
	    if(ret == false || ca->second != test_keys[i]){
		    fail++;
	    }
	  }
	  failed[tid] = fail;
  };
  const size_t chunk = N/numThreads;
  clear_cache();
  Timer t;
  for(int i=0; i<numThreads; i++){
    if(i != numThreads-1)
      insertingThreads.emplace_back(thread(insert, chunk*i, chunk*(i+1)));
    else 
      insertingThreads.emplace_back(thread(insert, chunk*i, N));
  }
  for(auto& t: insertingThreads) t.join();
  double elapsed = t.GetDurationNs();
  printf("Insertion %f Mops\n",(double)N/(elapsed/1000.0));


  clear_cache();
  t.Reset();
  for(int i=0; i<numThreads; i++){
	  if(i != numThreads-1)
	    searchingThreads.emplace_back(thread(search, chunk*i, chunk*(i+1), i));
	  else
	    searchingThreads.emplace_back(thread(search, chunk*i, N, i));
  }
  for(auto& t: searchingThreads) t.join();
  elapsed = t.GetDurationNs();
  printf("Search %f Mops\n", (double)N/(elapsed/1000.0));
  int failedSearch = 0;
  for(auto& v: failed) failedSearch += v;
  cout << failedSearch << " failedSearch" << "\n";
  assert(failedSearch == 0);
}

TEST(cas_hashtable, mt_int64_loadc) {
  HashTable ht(N);
  vector<thread> insertingThreads;
  vector<thread> searchingThreads;
  vector<int> failed(numThreads);
  auto insert = [&ht](int from, int to) {
	  for(int i=from; i<to; i++) {
	    ht.Put(test_keys[i], test_keys[i]);
	  }
  };
  auto search = [&ht, &failed](int from, int to, int tid){
	  int fail = 0;
	  for(int i=from; i<to; i++){
      uint64_t value;
	    bool ret = ht.Get(test_keys[i], value);
	    if(ret == false || value != test_keys[i]){
		    fail++;
	    }
	  }
	  failed[tid] = fail;
  };
  const size_t chunk = N/numThreads;
  clear_cache();
  Timer t;
  for(int i=0; i<numThreads; i++){
    if(i != numThreads-1)
      insertingThreads.emplace_back(thread(insert, chunk*i, chunk*(i+1)));
    else 
      insertingThreads.emplace_back(thread(insert, chunk*i, N));
  }
  for(auto& t: insertingThreads) t.join();
  double elapsed = t.GetDurationNs();
  printf("Insertion %f Mops\n",(double)N/(elapsed/1000.0));


  clear_cache();
  t.Reset();
  for(int i=0; i<numThreads; i++){
	  if(i != numThreads-1)
	    searchingThreads.emplace_back(thread(search, chunk*i, chunk*(i+1), i));
	  else
	    searchingThreads.emplace_back(thread(search, chunk*i, N, i));
  }
  for(auto& t: searchingThreads) t.join();
  elapsed = t.GetDurationNs();
  printf("Search %f Mops\n", (double)N/(elapsed/1000.0));
  int failedSearch = 0;
  for(auto& v: failed) failedSearch += v;
  cout << failedSearch << " failedSearch" << "\n";
  assert(failedSearch == 0);
}

TEST(cuckoo_hashmap, mt_int64_loadc) {
  libcuckoo::cuckoohash_map<uint64_t, uint64_t> ht(65536);
  vector<thread> insertingThreads;
  vector<thread> searchingThreads;
  vector<int> failed(numThreads);
  auto insert = [&ht](int from, int to) {
	  for(int i=from; i<to; i++) {
	    ht.insert(test_keys[i], test_keys[i]);
	  }
  };
  auto search = [&ht, &failed](int from, int to, int tid){
	  int fail = 0;
	  for(int i=from; i<to; i++){
      uint64_t value;
      int ret = ht.find(test_keys[i], value);
	    if(ret == false || value != test_keys[i]){
		    fail++;
	    }
	  }
	  failed[tid] = fail;
  };
  const size_t chunk = N/numThreads;
  clear_cache();
  Timer t;
  for(int i=0; i<numThreads; i++){
    if(i != numThreads-1)
      insertingThreads.emplace_back(thread(insert, chunk*i, chunk*(i+1)));
    else 
      insertingThreads.emplace_back(thread(insert, chunk*i, N));
  }
  for(auto& t: insertingThreads) t.join();
  double elapsed = t.GetDurationNs();
  printf("Insertion %f Mops\n",(double)N/(elapsed/1000.0));


  clear_cache();
  t.Reset();
  for(int i=0; i<numThreads; i++){
	  if(i != numThreads-1)
	    searchingThreads.emplace_back(thread(search, chunk*i, chunk*(i+1), i));
	  else
	    searchingThreads.emplace_back(thread(search, chunk*i, N, i));
  }
  for(auto& t: searchingThreads) t.join();
  elapsed = t.GetDurationNs();
  printf("Search %f Mops\n", (double)N/(elapsed/1000.0));
  int failedSearch = 0;
  for(auto& v: failed) failedSearch += v;
  cout << failedSearch << " failedSearch" << "\n";
  assert(failedSearch == 0);
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
 * --------------------
 * 8 threads: uint64->uint64
 * clht = cas > cuckoo > tbb
 */
