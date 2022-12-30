#include <gtest/gtest.h>
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <pthread.h>
#include <time.h>
#include "skiplist.h"

#define NUM_THREADS 4
#define TEST_COUNT 10'0000
SkipList<int, std::string> skipList(18);

void *insertElement(void *threadid)
{
  long tid;
  tid = (long)threadid;
  int tmp = TEST_COUNT / NUM_THREADS;
  for (int i = tid; i<TEST_COUNT; i+=NUM_THREADS)
  {
    int ret = skipList.insert(i, std::to_string(i));
    EXPECT_EQ(ret, 0);
  }
  pthread_exit(NULL);
}

void *getElement(void *threadid)
{
  long tid;
  tid = (long)threadid;
  int tmp = TEST_COUNT / NUM_THREADS;
  for (int i = tid; i<TEST_COUNT; i+=NUM_THREADS)
  {
    auto ret = skipList.find(i);
    EXPECT_TRUE(ret != nullptr);
    if(ret != nullptr) {
      EXPECT_EQ(ret->get_value(), std::to_string(i));
    }
  }
  pthread_exit(NULL);
}

TEST(skiplist, multi_write)
{
  pthread_t threads[NUM_THREADS];

  for (int i = 0; i < NUM_THREADS; i++)
  {
    int rc = pthread_create(&threads[i], NULL, insertElement, (void *)i);

    if (rc)
    {
      std::cout << "Error:unable to create thread," << rc << std::endl;
      exit(-1);
    }
  }
  for (int i = 0; i < NUM_THREADS; i++)
  {
    if (pthread_join(threads[i], NULL) != 0)
    {
      perror("pthread_create() error");
      exit(3);
    }
  }
}

TEST(skiplist, multi_read)
{
  pthread_t threads[NUM_THREADS];

  for (int i = 0; i < NUM_THREADS; i++)
  {
    int rc = pthread_create(&threads[i], NULL, getElement, (void *)i);

    if (rc)
    {
      std::cout << "Error:unable to create thread," << rc << std::endl;
      exit(-1);
    }
  }
  for (int i = 0; i < NUM_THREADS; i++)
  {
    if (pthread_join(threads[i], NULL) != 0)
    {
      perror("pthread_create() error");
      exit(3);
    }
  }
}

/**
 * 结果：
 * 1 thread:
 * skiplist.multi_write (44 ms) [43ms if no mutex protect]
 * skiplist.multi_read (35 ms)
 * 2 thread:
 * skiplist.multi_write (116 ms)
 * skiplist.multi_read (18 ms)
 * 4 thread:
 * skiplist.multi_write (124 ms)
 * skiplist.multi_read (12 ms)
 * 8 thread:
 * skiplist.multi_write (117 ms)
 * skiplist.multi_read (5 ms)
 * 16 thread:
 * skiplist.multi_write (136 ms)
 * skiplist.multi_read (3 ms)
*/