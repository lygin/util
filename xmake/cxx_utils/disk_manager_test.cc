#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "disk_manager.h"

TEST_CASE("ReadWritePageTest")
{
    char buf[PAGE_SIZE] = {0};
    char data[PAGE_SIZE] = {0};
    std::string db_file("test.db");
    DiskManager dm(db_file);
    strncpy(data, "test string", sizeof(data));
    dm.ReadPage(0, buf);

    dm.WritePage(0, data);
    dm.ReadPage(0, buf);
    LOG("%s", buf);
    CHECK(memcmp(buf, data, sizeof(buf)) == 0);

    memset(buf, 0, sizeof(buf));
    dm.WritePage(5, data);
    dm.ReadPage(5, buf);
    LOG("%s", buf);
    CHECK(memcmp(buf, data, sizeof(buf)) == 0);

    dm.ShutDown();
}

TEST_CASE("ReadWriteLogTest")
{
    char buf[16] = {0};
    char data[16] = {0};
    std::string db_file("test.db");
    DiskManager dm(db_file);
    strncpy(data, "test string", sizeof(data));

    dm.ReadLog(buf, sizeof(buf), 0);

    dm.WriteLog(data, sizeof(data));
    dm.ReadLog(buf, sizeof(buf), 0);
    LOG("%s", buf);
    CHECK(memcmp(buf, data, sizeof(buf)) == 0);

    dm.ShutDown();
}