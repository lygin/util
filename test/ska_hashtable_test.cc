#include <gtest/gtest.h>
#include <string>
#include <unordered_map>
#include <random>
#include "ska_hashtable.h"
#include "Random.h"

const int N = 10'0000;

std::vector<std::string> data;

TEST(INIT, INIT) {
  std::string str(128, ' ');
  RandomRng rd('0', 'z');
  for(int i=0; i<N; ++i) {
    for (char& c : str) {
      c = static_cast<char>(rd.rand());
      if (!std::isalnum(c)) c = 'a';  // 保证字符串只包含字母和数字
    }
    data.push_back(str);
  }
}

TEST(ska_hashtable_test, basic) {
    
    ska::flat_hash_map<std::string,std::string> map;
    for(int i=0; i<N; ++i) {
        map[data[i]] = data[i];
    }
}

TEST(stl_hashtable_test, basic) {
    std::unordered_map<std::string,std::string> map;
    for(int i=0; i<N; ++i) {
        map[data[i]] = data[i];
    }
}

/**
 * 结果：
 * ska_hashtable_test.basic (242 ms)
 * stl_hashtable_test.basic (213 ms)
 * ska_hashmap内存消耗比stl高80%
 * 总结:
 * 优先stl
*/

