#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "arena.h"
using namespace std;


#define DEBUG
#ifdef DEBUG
#define LOG(frm, argc...) {\
    printf("[%s : %d] ", __func__, __LINE__);\
    printf(frm, ##argc);\
    printf("\n");\
}
#else
#define LOG(frm, argc...)
#endif


TEST_CASE("arena") {
    Arena pool;
    int* p = reinterpret_cast<int*>(pool.Allocate(4));
    *p = 1<<31;
    LOG("%d", *p);
    CHECK(pool.MemoryUsage() == 4096+8);

    char* pua = pool.Allocate(1);
    LOG("%lx", (uint64_t)pua);
    CHECK((reinterpret_cast<uint64_t>(pua) & 7) != 0);
    pua = pool.Allocate(1);
    LOG("%lx", (uint64_t)pua);
    CHECK((reinterpret_cast<uint64_t>(pua) & 7) != 0);


    char* pa = pool.AllocateAligned(8);
    //8字节对齐
    LOG("%lx", (uint64_t)pa);
    CHECK((reinterpret_cast<uint64_t>(pa) & 7) == 0); 
}