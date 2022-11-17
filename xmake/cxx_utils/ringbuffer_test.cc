#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "ringbuffer.h"
#include "logging.h"
#include <thread>
using namespace std;

void produce(RingBuffer* buf, uint32_t num) {
    char nn[11];
    for(uint32_t i=0; i<num; ++i) {
        sprintf(nn, "%04u", i);
        buf->Put((uint8_t*)nn, 5);
        LOG_INFO("Put %s", nn);
    }   
}

void consume(RingBuffer* buf, uint32_t num) {
    char nn[11];
    char nnf[11];
    for(uint32_t i=0; i<num; ++i) {
        if (buf->Get((uint8_t*)nn, 5) == 0) {
            i--;
            continue; 
        }
        sprintf(nnf, "%04u", i);
        CHECK_EQ(memcmp(nn, nnf, 5), 0);
        LOG_INFO("Get %s", nn);
    }
}

TEST_CASE("spsc_test") {
    RingBuffer *buf = new RingBuffer(20000);
    LOG_INFO("%d", buf->size());
    REQUIRE_EQ(buf->capacity(), 32768);
    thread p(produce, buf, 1000);
    thread c(consume, buf, 1000);
    p.join();
    c.join();
    REQUIRE_EQ(buf->size(), 0);
    delete buf;
}