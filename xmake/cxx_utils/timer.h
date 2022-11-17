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
    double GetDurationSec()
    {
        auto tt = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::duration<double>>(tt-tp_).count();
    }
    double GetDurationMs()
    {
        auto tt = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(tt-tp_).count();
    }
    double GetDurationUs()
    {
        auto tt = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(tt-tp_).count();
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