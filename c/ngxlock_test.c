#include "ngxlock.h"

#include "pthread.h"
#include "logging.h"

ngx_atomic_t rwlock;
#define N (1<<20)
int aa[N];

void produce() {
    for(int i=0; i<N; i++) {
        //ngx_rwlock_wlock(&rwlock);
        ngx_spinlock_lock(&rwlock, 1, 2048);
        aa[i] = i;
        ngx_spinlock_unlock(&rwlock);
        //ngx_rwlock_unlock(&rwlock);
    }
}

void consume(void *i) {
    int *idx = (int*)i;
    while(1) {
        //ngx_rwlock_rlock(&rwlock);
        //LOG_INFO("%d", aa[*idx]);
        ngx_spinlock_lock(&rwlock, 1, 2048);
        (*idx)++;
        if((*idx) >= N-1) {
            ngx_rwlock_unlock(&rwlock);
            break;
        }
        ngx_spinlock_unlock(&rwlock);
        //ngx_rwlock_unlock(&rwlock);
    }
}

int main() {
    pthread_t t1,t2,t3;
    int idx = 0;
    LOG_INFO("start");
    pthread_create(&t1, NULL, produce, NULL);
    pthread_create(&t2, NULL, consume, &idx);
    pthread_create(&t3, NULL, consume, &idx);
    void* ret;
    pthread_join(t1, &ret);
    pthread_join(t2, &ret);
    pthread_join(t3, &ret);
    LOG_INFO("end");
    return 0;
}