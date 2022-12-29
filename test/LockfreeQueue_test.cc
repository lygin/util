#include <gtest/gtest.h>

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "rte_ring.h"
}

#include "LockfreeQueue.h"

const int RING_SIZE = (1<<20); //1MB
const long long N = 10'0000;

typedef struct cc_queue_node {
    int data;
} cc_queue_node_t;

struct rte_ring *r;
MPMCQueue<cc_queue_node_t*> mpmc_queue(RING_SIZE);

void *enqueue_fun(void *arg)
{
    long long mask = (long long)arg;
    int n = mask >> 1;
    int choice = mask & 1;
    
    int ret;
    cc_queue_node_t *p;

    for (int i = 0; i < n; i++) {
        p = (cc_queue_node_t *)malloc(sizeof(cc_queue_node_t));
        p->data = i;
        if(choice == 0) {
            ret = rte_ring_mp_enqueue(r, p);
            EXPECT_EQ(ret, 0);
        } else {
            mpmc_queue.emplace(p);
        }
    }
    return NULL;
}

void *dequeue_func(void *arg)
{
    int ret;
    long long sum = 0;
    long long mask = (long long)arg;
    int n = mask >> 1;
    int choice = mask & 1;
    cc_queue_node_t *p;
    for(int i=0; i<n; ++i) {
        p = NULL;
        if (choice == 0) {
            while(rte_ring_mc_dequeue(r, (void **)&p));
            EXPECT_TRUE(p != NULL);
            if (p != NULL) {
                sum += p->data;
                free(p);
            }
        } else {
            mpmc_queue.pop(p);
            sum += p->data;
            free(p);
        }
        
    }
    return (void*)sum;
}

TEST(rte_ring_test, basic) {
    int ret = 0;
    pthread_t pid1, pid2, pid3, pid4;

    r = rte_ring_create("test", RING_SIZE, 0);

    ASSERT_TRUE(r != NULL);
    long long mask = N << 1;
    ASSERT_EQ(pthread_create(&pid1, NULL, enqueue_fun, (void *)mask), 0);
    ASSERT_EQ(pthread_create(&pid2, NULL, enqueue_fun, (void *)mask), 0);
    ASSERT_EQ(pthread_create(&pid3, NULL, dequeue_func, (void *)mask), 0);
    ASSERT_EQ(pthread_create(&pid4, NULL, dequeue_func, (void *)mask), 0);

    pthread_join(pid1, NULL);
    pthread_join(pid2, NULL);
    void *ret1, *ret2;
    pthread_join(pid3, &ret1);
    pthread_join(pid4, &ret2);
    long long sum1 = (long long)ret1;
    long long sum2 = (long long)ret2;
    printf("sum 1:%lld sum 2:%lld\n", sum1, sum2);
    ASSERT_EQ(sum1+sum2, (N-1)*N);
    rte_ring_free(r);
}

TEST(mpmc_queue_test, basic) {
    int ret = 0;
    pthread_t pid1, pid2, pid3, pid4;
    long long mask = (N << 1) | 1;
    ASSERT_EQ(pthread_create(&pid1, NULL, enqueue_fun, (void *)mask), 0);
    ASSERT_EQ(pthread_create(&pid2, NULL, enqueue_fun, (void *)mask), 0);
    ASSERT_EQ(pthread_create(&pid3, NULL, dequeue_func, (void *)mask), 0);
    ASSERT_EQ(pthread_create(&pid4, NULL, dequeue_func, (void *)mask), 0);

    pthread_join(pid1, NULL);
    pthread_join(pid2, NULL);
    void *ret1, *ret2;
    pthread_join(pid3, &ret1);
    pthread_join(pid4, &ret2);
    long long sum1 = (long long)ret1;
    long long sum2 = (long long)ret2;
    printf("sum 1:%lld sum 2:%lld\n", sum1, sum2);
    ASSERT_EQ(sum1+sum2, (N-1)*N);
}

/**
 * 结果：
 * rte_ring_test.basic (71 ms)
 * mpmc_queue_test.basic (40 ms)
*/