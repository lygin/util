#ifndef __TC_SPIN_LOCK_H
#define __TC_SPIN_LOCK_H
#include <atomic>
#include <memory>
using namespace std;
/**
 * 自旋锁
 * 不能阻塞wait, 只能快速加解锁, 适用于锁粒度非常小的情况, 减小线程切换的开销
 * 不支持trylock
 */
class SpinLock
{
public:

	SpinLock();
	virtual ~SpinLock();

	void lock() const;
    bool tryLock() const;
    void unlock() const;

private:

	SpinLock(const SpinLock&) = delete;
	SpinLock(SpinLock&&) = delete;
	SpinLock& operator=(const SpinLock&) = delete;
	SpinLock& operator=(SpinLock&&) = delete;

private:

	mutable std::atomic_flag _flag;
};

#endif
