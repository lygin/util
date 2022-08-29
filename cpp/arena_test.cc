#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "arena.h"
#include "timer.h"
#include <mimalloc-2.0/mimalloc.h>

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

constexpr int times = 100'0000;

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
    //glibc malloc 
    Timer t;
    ticks t1, t2;
    t1 = getticks();
    for(int i=0; i<times; ++i) {
        void* x = malloc(8);
        free(x);
    }
    t2 = getticks();
    LOG("malloc %llu %fs", t2-t1, t.GetDurationSec());
    //arena
    t.Reset();
    t1 = getticks();
    for(int i=0; i<times; ++i) {
        pool.AllocateAligned(8);
    }
    t2 = getticks();
    LOG("arena %llu %fs", t2-t1, t.GetDurationSec());
    //mimalloc
    t.Reset();
    t1 = getticks();
    for(int i=0; i<times; ++i) {
        void* x = mi_malloc_aligned(8, 8);
        free(x);
    }
    t2 = getticks();
    LOG("mimalloc %llu %fs", t2-t1, t.GetDurationSec());
    //new
    t.Reset();
    t1 = getticks();
    for(int i=0; i<times; ++i) {
        void** x = new void*;
        delete x;
    }
    t2 = getticks();
    LOG("new %llu %fs", t2-t1, t.GetDurationSec());

}