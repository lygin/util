#pragma once
#include <iostream>
#include <chrono>
#include <exception>

class Timer {
public:
    Timer(): tp_(std::chrono::steady_clock::now()) {}
    void Reset()
    {
        tp_ = std::chrono::steady_clock::now();
    }
    std::chrono::duration<double> GetDuration()
    {
        return std::chrono::steady_clock::now() - tp_;
    }
    double GetDurationSec()
    {
        return GetDuration().count();
    }

private:
    std::chrono::steady_clock::time_point tp_;
};