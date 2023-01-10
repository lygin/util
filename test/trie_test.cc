#include "trie.h"
#include <gtest/gtest.h>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

TEST(Trie, basic)
{
  Trie<std::string> t;

  t.insert("wargame");
  t.insert("wombat");
  t.insert("wolfram");
  t.insert("world");
  t.insert("work");

  ASSERT_TRUE(t.size() == 5);
  ASSERT_TRUE(t.find("war") == false);
  ASSERT_TRUE(t.find("wargame") == true);

  auto candidates = t.complete("wo");
  ASSERT_TRUE(candidates.size() == 4);

  candidates = t.complete("warg");
  ASSERT_TRUE(candidates.size() == 1);
  ASSERT_TRUE(*candidates[0] == "wargame");

  {
    Trie<std::string, int> count;
    count["/dir1/file1"] = 10;
    count["/dir1/file3"] = 30;
    count["/dir1/file2"] = 20;
    count["/dir2/"];
    ASSERT_TRUE(count.size() == 4);
    ASSERT_TRUE(count["/dir1/file3"] == 30);
    ASSERT_TRUE(count.complete("/dir1/").size() == 3);
  }

  {
    using path = std::vector<std::string>;
    path d1d1 = path{"dir1", "dir1"};
    path d1d2 = path{"dir1", "dir2"};
    path d1d3 = path{"dir1", "dir3"};
    path d1 = path{"dir1"};
    path d2 = path{"dir2"};
    Trie<path, std::string> files;
    files[d1d1] = "10";
    files[d1d3] = "30";
    files[d1d2] = "20";
    files[d2];
    ASSERT_TRUE(files.size() == 4);
    ASSERT_TRUE(files[d1d3] == "30");
    ASSERT_TRUE(files.complete(d1).size() == 3);
  }
}