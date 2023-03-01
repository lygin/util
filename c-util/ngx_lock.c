#include "ngx_lock.h"

#define NGX_RWLOCK_SPIN   2048
#define NGX_RWLOCK_WLOCK  ((unsigned long)-1)

// 读写均衡，写会等待所有读锁释放；读会等待写锁释放
void ngx_rwlock_wlock(ngx_atomic_t *lock)
{
    ngx_uint_t  i, n;
    for ( ;; ) {
        if (*lock == 0 && __sync_bool_compare_and_swap(lock, 0, NGX_RWLOCK_WLOCK)) {
            return;
        }
        /*随着等待的次数越来越多，检查lock频率会越来越小。*/     
        for (n = 1; n < NGX_RWLOCK_SPIN; n <<= 1) {
            for (i = 0; i < n; i++) {
                __asm__ ("pause");
            }
            if (*lock == 0 &&  __sync_bool_compare_and_swap(lock, 0, NGX_RWLOCK_WLOCK)) {
                return;
            }
        }
        /* 让出CPU */
        sched_yield();
    }
}

void ngx_rwlock_rlock(ngx_atomic_t *lock)
{
    ngx_uint_t         i, n;
    unsigned long  old_readers;
    for ( ;; ) {
        old_readers = *lock;
        if (old_readers != NGX_RWLOCK_WLOCK && __sync_bool_compare_and_swap(lock, old_readers, old_readers + 1)) {
            return;
        }
        for (n = 1; n < NGX_RWLOCK_SPIN; n <<= 1) {
            for (i = 0; i < n; i++) {
                __asm__ ("pause");
            }
            old_readers = *lock;
            if (old_readers != NGX_RWLOCK_WLOCK && __sync_bool_compare_and_swap(lock, old_readers, old_readers + 1)) {
                return;
            }
        }
        sched_yield();
    }
}

void ngx_rwlock_unlock(ngx_atomic_t *lock)
{
    unsigned long  old_readers;
    old_readers = *lock;
    if (old_readers == NGX_RWLOCK_WLOCK) {
        *lock = 0;
        return;
    }
    for ( ;; ) {
        if (__sync_bool_compare_and_swap(lock, old_readers, old_readers - 1)) {
            return;
        }
        old_readers = *lock;
    }
}

/*
 * para:
 * lock: 原子变量表达的锁，当lock值为0时表示锁是被释放的，而lock值不为0时则表示锁已经被某个进程持有了；
 * value: 表示希塑当锁没有被任何进程持有时，把lock值设为value表示当前进程持有了锁；
 * spin: 如果lock不为0，则pause一会儿，再次检查，如果检查log2(spin)次还没有获取到锁，则让出CPU控制权
 */
void ngx_spinlock_lock(ngx_atomic_t *lock, long value, ngx_uint_t spin)
{
    ngx_uint_t  i, n;
    for ( ;; ) {
        if (*lock == 0 && __sync_bool_compare_and_swap(lock, 0, value)) {
            return;
        }
        /*
        在多处理器下，更好的做法是当前进程不要立刻“让出”正在使用的CPU处理器，而是等待一段时间，看看其他处理器上的
        进程是否会释放锁，这会减少进程间切换的次数
        */
        for (n = 1; n < spin; n <<= 1) {
            /*随着等待的次数越来越多，检查lock频率会越来越小。*/            
            for (i = 0; i < n; i++) {
                /*pause是在许多架构体系中专门为了自旋锁而提供的指令，它会告诉CPU现在处于自旋锁等待状悉
                CPU会将自己置于节能状态，降低功耗。*/
                __asm__ ("pause");
            }
            if (*lock == 0 && __sync_bool_compare_and_swap(lock, 0, value)) {
                return;
            }
        }
        /* 让出CPU */
        sched_yield();
    }
}