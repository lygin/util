#include <gtest/gtest.h>
#include "page_cache.h"

class PageCacheTest : public ::testing::Test
{
public:
  static constexpr char const *db_name = "test.db";
  static constexpr size_t cache_size = 16;
  DiskManager *disk_manager;
  PageCache *pg_cache;

  PageCacheTest() : disk_manager(new DiskManager(db_name)),
                    pg_cache(new PageCache(cache_size, disk_manager)) {}

protected:
  void TearDown() override
  {
    remove("test.db");
    remove("test.log");
  }
};

TEST_F(PageCacheTest, basic)
{
  // read write
  pg_cache->WritePage(1, 0, "good boy");

  char *ret = pg_cache->ReadPage(1, 0);
  ASSERT_TRUE(memcmp(ret, "good boy", 8) == 0);
  pg_cache->WritePage(1, 8, "good boy");
  ASSERT_TRUE(memcmp(ret, "good boygood boy", 16) == 0);
  ASSERT_EQ(pg_cache->PageInCacheNum(), 1);
  pg_cache->FlushAllPages();
  pg_cache->DeletePage(1);

  ret = pg_cache->ReadPage(1, 0);
  ASSERT_EQ(pg_cache->PageInCacheNum(), 1);
  ASSERT_TRUE(memcmp(ret, "good boygood boy", 16) == 0);
}

TEST_F(PageCacheTest, lowApi)
{
  auto ent = pg_cache->FetchPage(0);
  Page *pg = (Page *)ent->value;
  snprintf(pg->GetData(), PAGE_SIZE, "hello");
  ASSERT_TRUE(pg_cache->PageInCacheNum() == 1);
  pg_cache->ReleasePage(ent, true);

  pg_cache->FlushAllPages();
  ASSERT_TRUE(pg_cache->PageInCacheNum() == 1);

  pg_cache->DeletePage(0);
  ASSERT_EQ(pg_cache->PageInCacheNum(), 0);

  ent = pg_cache->FetchPage(0);
  pg = (Page *)ent->value;
  ASSERT_TRUE(strncmp(pg->GetData(), "hello", 5) == 0);
  pg_cache->ReleasePage(ent, false);
}
