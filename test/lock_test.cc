#include <gtest/gtest.h>

#include <thread>
#include <map>
#include "xxhash.h"

#include <pthread.h>
#include "lock.h"

using namespace std;
map<int, int> rbtree;
const int N = 10'0000;

CASRWLock caslock;
pthread_spinlock_t spinlock;
RWLock trwlock;
RWMutex rocks_rwlock;
mutex mtx;

void putp()
{
	for (int i = 0; i < N; i++)
	{
		pthread_spin_lock(&spinlock);
		rbtree.emplace(i, i);
		pthread_spin_unlock(&spinlock);
	}
}
void getp()
{
	for (int i = 0; i < N; i++)
	{
		pthread_spin_lock(&spinlock);
		EXPECT_TRUE(rbtree.find(i) == rbtree.end() || rbtree[i] == i);
		pthread_spin_unlock(&spinlock);
	}
}

void writen()
{
	for (int i = 0; i < N; i++)
	{
		caslock.Wlock();
		rbtree.emplace(i, i);
		caslock.Unlock();
	}
}
void readn()
{
	for (int i = 0; i < N; i++)
	{
		caslock.Wlock();
		EXPECT_TRUE(rbtree.find(i) == rbtree.end() || rbtree[i] == i);
		caslock.Unlock();
	}
}


void writet()
{
	for (int i = 0; i < N; i++)
	{
		trwlock.WLock();
		rbtree.emplace(i, i);
		trwlock.WUnlock();
	}
}
void readt()
{
	for (int i = 0; i < N; i++)
	{
		trwlock.RLock();
		EXPECT_TRUE(rbtree.find(i) == rbtree.end() || rbtree[i] == i);
		trwlock.RUnlock();
	}
}

void writerocks()
{
	for (int i = 0; i < N; i++)
	{
		rocks_rwlock.WriteLock();
		rbtree.emplace(i, i);
		rocks_rwlock.WriteUnlock();
	}
}

void readrocks()
{
	for (int i = 0; i < N; i++)
	{
		rocks_rwlock.ReadLock();
		EXPECT_TRUE(rbtree.find(i) == rbtree.end() || rbtree[i] == i);
		rocks_rwlock.ReadUnlock();
	}
}

void writemtx()
{
	for (int i = 0; i < N; i++)
	{
		mtx.lock();
		rbtree.emplace(i, i);
		mtx.unlock();
	}
}
void readmtx()
{
	for (int i = 0; i < N; i++)
	{
		mtx.lock();
		EXPECT_TRUE(rbtree.find(i) == rbtree.end() || rbtree[i] == i);
		mtx.unlock();
	}
}


TEST(pthread_lock, spin_lock)
{
	rbtree = map<int, int>();
	pthread_spin_init(&spinlock, 0);
	thread t1(putp);
	thread t2(getp);
	t1.join();
	t2.join();
}

TEST(ngx_lock, rwlock)
{
	rbtree = map<int, int>();
	thread t1(writen);
	thread t2(readn);
	t1.join();
	t2.join();
}
TEST(tars_rwlock, rwlock)
{
	rbtree = map<int, int>();
	thread t1(writet);
	thread t2(readt);
	t1.join();
	t2.join();
}
TEST(rocks_rwlock, rwlock)
{
	rbtree = map<int, int>();
	thread t1(writerocks);
	thread t2(readrocks);
	t1.join();
	t2.join();
}


TEST(stl_mutex, mutex)
{
	rbtree = map<int, int>();
	thread t1(writemtx);
	thread t2(readmtx);
	t1.join();
	t2.join();
}

uint64_t INT_hasher(const int &x) {
	int temp = x;
	return XXH3_64bits(&temp, 4);
}
Striped<RWMutex, int> striped_rwmutex(128, INT_hasher);

void writestriped()
{
	for (int i = 0; i < N; i++)
	{
		striped_rwmutex.get(i)->WriteLock();
		rbtree.emplace(i, i);
		striped_rwmutex.get(i)->WriteUnlock();
	}
}

void readstriped()
{
	for (int i = 0; i < N; i++)
	{
		striped_rwmutex.get(i)->ReadLock();
		EXPECT_TRUE(rbtree.find(i) == rbtree.end() || rbtree[i] == i);
		striped_rwmutex.get(i)->ReadUnlock();
	}
}

TEST(striped_rwmutex, rwlock) {
	rbtree = map<int, int>();
	thread t1(writestriped);
	thread t2(readstriped);
	t1.join();
	t2.join();
}

Striped<SpinMutex, int> striped_spinmutex(128, INT_hasher);

void writestripedspin()
{
	for (int i = 0; i < N; i++)
	{
		striped_spinmutex.get(i)->lock();
		rbtree.emplace(i, i);
		striped_spinmutex.get(i)->unlock();
	}
}

void readstripedspin()
{
	for (int i = 0; i < N; i++)
	{
		striped_spinmutex.get(i)->lock();
		EXPECT_TRUE(rbtree.find(i) == rbtree.end() || rbtree[i] == i);
		striped_spinmutex.get(i)->unlock();
	}
}

TEST(striped_spinmutex, spinlock) {
	rbtree = map<int, int>();
	thread t1(writestripedspin);
	thread t2(readstripedspin);
	t1.join();
	t2.join();
}

/**
 * 结果：
 * ngx_lock.spin_lock (62 ms)
 * ngx_lock.rwlock (58 ms)
 * pthread_lock.spin_lock (61 ms)
 * pthread_lock.rwlock (634 ms)
 * rocks_rwlock.rwlock (539 ms)
 * tars_rwlock.rwlock (105 ms)
 * stl_mutex.mutex (46 ms)
 * 
 * striped_rwmutex.rwlock (41 ms)
 * striped_spinmutex.spinlock (38 ms)
 */