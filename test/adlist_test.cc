#include <gtest/gtest.h>
extern "C" {
#include "adlist.h"
}

TEST(AdlistTest, listAddNodeHeadTail) {
  list *adlist = listCreate();
  int x = 0;
  listAddNodeHead(adlist, (void*)x);
  int y = 1;
  listAddNodeTail(adlist, (void*)y);
  EXPECT_EQ(0, (uint64_t)listNodeValue(adlist->head));
  EXPECT_EQ(1, (uint64_t)listNodeValue(adlist->tail));
}