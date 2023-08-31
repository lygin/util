#pragma once
#include <iostream>
#include <chrono>
#include <exception>
#include <cstring>

class Timer
{
public:
	Timer() : tp_(std::chrono::high_resolution_clock::now()) {}
	void Reset()
	{
		tp_ = std::chrono::high_resolution_clock::now();
	}
	double GetDurationSec()
	{
		auto tt = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::duration<double>>(tt - tp_).count();
	}
	double GetDurationMs()
	{
		auto tt = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(tt - tp_).count();
	}
	double GetDurationUs()
	{
		auto tt = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(tt - tp_).count();
	}
	double GetDurationNs()
	{
		auto tt = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(tt - tp_).count();
	}

private:
	std::chrono::high_resolution_clock::time_point tp_;
};

/* time = ticks / freq */
static __inline__ uint64_t getticks()
{
	uint32_t a, d;

	asm volatile("rdtsc"
							 : "=a"(a), "=d"(d));
	return (((uint64_t)a) | (((uint64_t)d) << 32));
}

__always_inline int parseLine(char* line){
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char* p = line;
    while (*p <'0' || *p > '9') p++;
    line[i-3] = '\0';
    i = atoi(p);
    return i;
}
//Note: this value is in KB!
__always_inline int getMem(){ 
    FILE* file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmRSS:", 6) == 0){
            result = parseLine(line);
            break;
        }
    }
    fclose(file);
    return result;
}