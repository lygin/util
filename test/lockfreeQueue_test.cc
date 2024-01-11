#include <gtest/gtest.h>

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
}

#include "lockfreeQueue.h"
#include "timer.h"
#include "concurrentqueue.h"
#include <atomic>
#include <thread>
#include <future>

const int RING_SIZE = 1<<20; //1MB
const long long N = 10'0000; //testnums per thread
const int produce_threads = 2;
const int consume_threads = 2; //equal to produce_threads

struct cc_queue_node_t {
    int data;
};

MPMCRing<cc_queue_node_t*> mpmc_queue(RING_SIZE);
moodycamel::ConcurrentQueue<cc_queue_node_t*> conq(RING_SIZE);

void produce(int c) {
    cc_queue_node_t *p;
    int ret = 0;
    for(int i=0; i< N; ++i) {
        p = (cc_queue_node_t *)malloc(sizeof(cc_queue_node_t));
        p->data = i;
        switch (c)
        {
        case 0:
            EXPECT_EQ(ret, 0);
            break;
        case 1:
            mpmc_queue.emplace(p);
            break;
        case 2:
            ret = conq.enqueue(p);
            EXPECT_EQ(ret, 1);
            break;
        default:
            break;
        }
        
    }
}

long long consume(int c) {
    cc_queue_node_t *p;
    long long sum = 0;
    for (int i=0; i< N; ++i) {
        p = NULL;
        switch (c)
        {
        case 0:
            break;
        case 1:
            while(mpmc_queue.try_pop(p) == false);
            break;
        case 2:
            while(conq.try_dequeue(p) == false);
            break;
        default:
            break;
        }
        sum += p->data;
        free(p);
    }
    return sum;
}



TEST(mpmc_queue, basic) {
    Timer tm;
    for(int i = 0; i <produce_threads; ++i) {
        (void)std::async(produce, 1);
    }
    long long sum = 0;
    for(int i = 0; i<consume_threads; ++i) {
        sum += std::async(consume, 1).get();
    }
    ASSERT_EQ(sum, (N-1)*N*produce_threads/2);
    printf("IOPS: %fMops\n", (double)N*(produce_threads+consume_threads)/tm.GetDurationUs());
}

TEST(moodycamel_conqueue, basic) {
    Timer tm;
    for(int i = 0; i <produce_threads; ++i) {
        (void)std::async(produce, 2);
    }
    long long sum = 0;
    for(int i = 0; i<consume_threads; ++i) {
        sum += std::async(consume, 2).get();
    }
    ASSERT_EQ(sum, (N-1)*N*produce_threads/2);
    printf("IOPS: %fMops\n", (double)N*(produce_threads+consume_threads)/tm.GetDurationUs());
}

/**
 * results:
 * 2p2c:
 * rte_ring.basic IOPS: 5.493300Mops
 * mpmc_queue.basic IOPS: 6.897560Mops
 * moodycamel_conqueue.basic IOPS: 3.312381Mops
 * 4p4c
 * rte_ring.basic IOPS: 6.877428Mops
 * mpmc_queue.basic IOPS: 6.663749Mops
 * moodycamel_conqueue.basic IOPS: 3.150490Mops
 * 8p8c
 * rte_ring.basic IOPS: 6.402183Mops
 * mpmc_queue.basic IOPS: 6.602323Mops
 * moodycamel_conqueue.basic IOPS: 3.338522Mops
*/