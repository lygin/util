#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "rte_memcpy.h"
#include "get_tick.h"

const int N = 1000000;

int main() {
    char a[64];
    char b[64];
    ticks t1 = getticks();
    for(int i=0; i<N; ++i) {
        //rte_mov64(a,b);
    }
    ticks t2 = getticks();
    printf("%llu\n", t2-t1);
    t1 = getticks();
    for(int i=0; i<N; ++i) {
        memcpy(a,b, 64);
    }
    t2 = getticks();
    printf("%llu\n", t2-t1);
}