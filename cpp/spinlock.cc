#include "spinlock.h"

#include <thread>
#include <iostream>
#include <cassert>
using namespace std;

#define TRYS_COUNT 10
#define TRYS_SLEEP 1

SpinLock::SpinLock()
{
    _flag.clear(std::memory_order_release);
}

SpinLock::~SpinLock()
{
}

void SpinLock::lock() const
{
    for (size_t i = 1; _flag.test_and_set(std::memory_order_acquire); i++)
    {
    	if(i % TRYS_COUNT == 0)
		{
    		std::this_thread::sleep_for(std::chrono::milliseconds(TRYS_SLEEP));
		}
    	else
		{
			std::this_thread::yield();
		}
    }
}

void SpinLock::unlock() const
{
    _flag.clear(std::memory_order_release);
}

bool SpinLock::tryLock() const
{
    int trys = TRYS_COUNT;
    for (; trys > 0 && _flag.test_and_set(std::memory_order_acquire); --trys)
    {
		std::this_thread::yield();
    }

    if (trys > 0)
        return true;

    return false;
}