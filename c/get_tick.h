#ifndef _GET_TICK_H_
#define _GET_TICK_H_
#include <stdint.h>
typedef unsigned long long ticks;
static __inline__ ticks getticks(void)
{
    uint32_t a, d;

    asm volatile("rdtsc" : "=a" (a), "=d" (d));
    return (((ticks)a) | (((ticks)d) << 32));
}
#endif