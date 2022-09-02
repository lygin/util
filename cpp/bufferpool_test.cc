#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "bufferpool.h"

TEST_CASE("SampleTest") {
    const std::string db_name = "test.db";
    //2的整数倍
    const size_t buffer_pool_size = 16;
    auto disk_manager = new DiskManager(db_name);
    auto bp = new BufferPoolManager(buffer_pool_size, disk_manager);

    //直接拿第一页写数据，LOG(read from disk)
    auto ent = bp->FetchPage(0);
    Page* pg = (Page*)ent->value;
    snprintf(pg->GetData(), PAGE_SIZE, "hello");
    REQUIRE(bp->TotalElem() == 1);
    bp->ReleasePage(ent, true);
    bp->FlushAllPages();
    REQUIRE(bp->TotalElem() == 1);
    //继续拿第一页，LOG(cache hit)
    ent = bp->FetchPage(0);
    pg = (Page*)ent->value;
    LOG("%s", pg->GetData());
    REQUIRE(strncmp(pg->GetData(), "hello", 5) == 0);
    //删除第一页再拿，LOG(read from disk)
    bp->DeletePage(0);
    REQUIRE_EQ(bp->TotalElem(), 0);
    ent = bp->FetchPage(0);
    pg = (Page*)ent->value;
    LOG("%s", pg->GetData());
    REQUIRE_EQ(bp->TotalElem(), 1);
}