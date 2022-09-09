#pragma once
#include <iostream>
#include <chrono>
#include <exception>

class Timer {
public:
    Timer(): tp_(std::chrono::high_resolution_clock::now()) {}
    void Reset()
    {
        tp_ = std::chrono::high_resolution_clock::now();
    }
    std::chrono::duration<double> GetDuration()
    {
        return std::chrono::high_resolution_clock::now() - tp_;
    }
    double GetDurationSec()
    {
        return GetDuration().count();
    }

private:
    std::chrono::high_resolution_clock::time_point tp_;
};

typedef unsigned long long ticks;
static __inline__ ticks getticks(void)
{
    uint32_t a, d;

    asm volatile("rdtsc" : "=a" (a), "=d" (d));
    return (((ticks)a) | (((ticks)d) << 32));
}