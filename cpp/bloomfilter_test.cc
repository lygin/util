#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "bloomfilter.h"
#include "doctest.h"
#include <memory>
using namespace std;

#ifdef DEBUG
#define LOG(frm, argc...)                                                      \
  {                                                                            \
    printf("[%s : %d] ", __func__, __LINE__);                                  \
    printf(frm, ##argc);                                                       \
    printf("\n");                                                              \
  }
#else
#define LOG(frm, argc...)
#endif

TEST_CASE("bloom") {
  string filter;
  shared_ptr<BloomFilter> bf = make_shared<BloomFilter>(10);
  vector<string> keys;
  for (int i = 0; i < 26; ++i) {
    char c = i + 'A';
    string x = "xx" + string(1, c);
    LOG("%s", x.c_str());
    keys.emplace_back(x);
  }

  vector<Slice> key_slices;
  for (auto &&key : keys) {
    key_slices.emplace_back(key);
  }
  bf->CreateFilter(&key_slices[0], key_slices.size(), &filter);
  keys.clear();
  key_slices.clear();
  REQUIRE(filter.size() == (26 * 10 + 7) / 8 + 1);

  CHECK(bf->KeyMayMatch("xxA", filter) == true);
  CHECK(bf->KeyMayMatch("xxZ", filter) == true);
  CHECK(bf->KeyMayMatch("xxa", filter) == false);
  CHECK(bf->KeyMayMatch("xxz", filter) == false);
}