#include <gtest/gtest.h>
#include <string>
#include <unordered_map>
#include <random>
#include "ska_hashtable.h"

const int N = 500000;

std::random_device rd;
std::mt19937 gen(rd());
std::vector<std::string> data;

TEST(INIT, INIT) {
  std::string str(128, ' ');
  for(int i=0; i<N; ++i) {
    std::uniform_int_distribution<int> dist('0', 'z');
    for (char& c : str) {
      c = static_cast<char>(dist(gen));
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

