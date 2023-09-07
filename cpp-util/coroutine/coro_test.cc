#include "coro.h"

#include <unistd.h>

int main() {
    Corot::Sche *sche = new Corot::Sche(4);
    coro_run(sche, [&sche]() {
        while(true) {
            printf("aaa\n");
            sleep(1);
            co_yield();
            printf("aaa2\n");
        } 
    });
    coro_run(sche, [&sche]() {
        while(true) {
            printf("bbb\n");
            sleep(1);
            co_yield();
            printf("bbb2\n");
        } 
    });
    sleep(10);
    return 0;
}