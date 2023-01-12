#include <gtest/gtest.h>

#include <thread>
#include <map>

extern "C"
{
#include "ngx_lock.h"
#include <pthread.h>
}
#include "tars_rwlock.h"
#include "rwlock.h"

using namespace std;
map<int, int> rbtree;
const int N = 10'0000;

ngx_atomic_t rwlock;
pthread_spinlock_t spinlock;
pthread_rwlock_t prwlock;
RWLock trwlock;
Rwlock msk_rwlock;
mutex mtx;

void putn()
{
	for (int i = 0; i < N; i++)
	{
		ngx_spinlock_lock(&rwlock, 1, 1024);
		rbtree.emplace(i, i);
		ngx_spinlock_unlock(&rwlock);
	}
}
void getn()
{
	for (int i = 0; i < N; i++)
	{
		ngx_spinlock_lock(&rwlock, 1, 1024);
		ASSERT_TRUE(rbtree[i] == i || rbtree[i] == 0);
		ngx_spinlock_unlock(&rwlock);
	}
}

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
		ASSERT_TRUE(rbtree[i] == i || rbtree[i] == 0);
		pthread_spin_unlock(&spinlock);
	}
}

void writen()
{
	for (int i = 0; i < N; i++)
	{
		ngx_rwlock_wlock(&rwlock);
		rbtree.emplace(i, i);
		ngx_rwlock_unlock(&rwlock);
	}
}
void readn()
{
	for (int i = 0; i < N; i++)
	{
		ngx_rwlock_rlock(&rwlock);
		ASSERT_TRUE(rbtree[i] == i || rbtree[i] == 0);
		ngx_rwlock_unlock(&rwlock);
	}
}

void writep()
{
	for (int i = 0; i < N; i++)
	{
		pthread_rwlock_wrlock(&prwlock);
		rbtree.emplace(i, i);
		pthread_rwlock_unlock(&prwlock);
	}
}
void readp()
{
	for (int i = 0; i < N; i++)
	{
		pthread_rwlock_rdlock(&prwlock);
		ASSERT_TRUE(rbtree[i] == i || rbtree[i] == 0);
		pthread_rwlock_unlock(&prwlock);
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
		ASSERT_TRUE(rbtree[i] == i || rbtree[i] == 0);
		trwlock.RUnlock();
	}
}

void writemsk()
{
	for (int i = 0; i < N; i++)
	{
		msk_rwlock.WLock();
		rbtree.emplace(i, i);
		msk_rwlock.Unlock();
	}
}
void readmsk()
{
	for (int i = 0; i < N; i++)
	{
		msk_rwlock.RLock();
		ASSERT_TRUE(rbtree[i] == i || rbtree[i] == 0);
		msk_rwlock.Unlock();
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
		ASSERT_TRUE(rbtree[i] == i || rbtree[i] == 0);
		mtx.unlock();
	}
}

TEST(ngx_lock, spin_lock)
{
	rbtree = map<int, int>();
	thread t1(putn);
	thread t2(getn);
	t1.join();
	t2.join();
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

TEST(pthread_lock, rwlock)
{
	rbtree = map<int, int>();
	pthread_rwlock_init(&prwlock, NULL);
	thread t1(writep);
	thread t2(readp);
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

TEST(msk_rwlock, rwlock)
{
	rbtree = map<int, int>();
	thread t1(writemsk);
	thread t2(readmsk);
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

/**
 * 结果：
 * ngx_lock.spin_lock (347 ms)
 * ngx_lock.rwlock (256 ms)
 * msk_rwlock.rwlock (230 ms)
 * pthread_lock.spin_lock (261 ms)
 * pthread_lock.rwlock (1225 ms)
 * tars_rwlock.rwlock (494 ms)
 * stl_mutex.mutex (417 ms)
 * 总结：
 * 考虑性能优先使用 msk_rwlock ngx_rwlock pthread_spinlock
 * 考虑CPU占用率优先使用 tars_rwlock stl_mutex/pthread_mutex
 */