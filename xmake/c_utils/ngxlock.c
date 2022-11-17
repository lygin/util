#include "ngxlock.h"

#define NGX_RWLOCK_SPIN   2048
#define NGX_RWLOCK_WLOCK  ((unsigned long)-1)
const int ngx_ncpu = 8;

void ngx_rwlock_wlock(ngx_atomic_t *lock)
{
    ngx_uint_t  i, n;

    for ( ;; ) {

        if (*lock == 0 && __sync_bool_compare_and_swap(lock, 0, NGX_RWLOCK_WLOCK)) {
            return;
        }

        if (ngx_ncpu > 1) {
            //spin 11 次
            for (n = 1; n < NGX_RWLOCK_SPIN; n <<= 1) {

                for (i = 0; i < n; i++) {
                    __asm__ ("pause");
                }

                if (*lock == 0
                    && __sync_bool_compare_and_swap(lock, 0, NGX_RWLOCK_WLOCK))
                {
                    return;
                }
            }
        }

        sched_yield();
    }
}


void ngx_rwlock_rlock(ngx_atomic_t *lock)
{
    ngx_uint_t         i, n;
    unsigned long  readers;

    for ( ;; ) {
        readers = *lock;

        if (readers != NGX_RWLOCK_WLOCK
            && __sync_bool_compare_and_swap(lock, readers, readers + 1))
        {
            return;
        }

        if (ngx_ncpu > 1) {

            for (n = 1; n < NGX_RWLOCK_SPIN; n <<= 1) {

                for (i = 0; i < n; i++) {
                    __asm__ ("pause");
                }

                readers = *lock;

                if (readers != NGX_RWLOCK_WLOCK
                    && __sync_bool_compare_and_swap(lock, readers, readers + 1))
                {
                    return;
                }
            }
        }

        sched_yield();
    }
}


void ngx_rwlock_unlock(ngx_atomic_t *lock)
{
    unsigned long  readers;

    readers = *lock;

    if (readers == NGX_RWLOCK_WLOCK) {
        *lock = 0;
        return;
    }

    for ( ;; ) {

        if (__sync_bool_compare_and_swap(lock, readers, readers - 1)) {
            return;
        }

        readers = *lock;
    }
}

/*
释放锁时需要Nginx模块通过ngx_atomic_cmp_set方法将原子变量lock值设为0。
可以看到，ngx_spinlock方法是非常高效的自旋锁，它充分考虑了单处理器和多处理器的系统，对于持有锁时间非常短的场景很有效率


它有3个参数，其中，lock参数就是原子变量表达的锁，当lock值为0时表示锁是被释放的，而lock值不为0时则表示锁已经被某个进程持有了；
value参数表示希塑当锁没有被任何进程持有时（也就是lock值为0），把lock值设为value表示当前进程持有了锁；第三个参数spin表示在多处
理器系统内，当ngx_spinlock方法没有拿到锁时，当前进程在内核的一次调度中，该方法等待其他处理器释放锁的时间
*/

//如果lock为0，表示可以获取锁，然后把lock置为value，如果lock不为0，则循环调度一会儿，再次检查，如果spin次循环还没有获取到锁，则让出CPU控制权
void ngx_spinlock_lock(ngx_atomic_t *lock, long value, ngx_uint_t spin)
{
    ngx_uint_t  i, n;

    //无法获取锁时进程的代码将一直在这个循环中执行
    for ( ;; ) {

        //lock为O时表示锁是没有被其他进程持有的，这时将lock值设为value参数表示当前进程持有了锁
        if (*lock == 0 && __sync_bool_compare_and_swap(lock, 0, value)) {
            return;
        }

        //ngx_ncpu是处理器的个数，当它大于1时表示处于多处理器系统中
        if (ngx_ncpu > 1) {

             /*
                在多处理器下，更好的做法是当前进程不要立刻“让出”正在使用的CPU处理器，而是等待一段时间，看看其他处理器上的
                进程是否会释放锁，这会减少进程间切换的次数
                */
            for (n = 1; n < spin; n <<= 1) {
    
                /*
                    注意，随着等待的次数越来越多，实际去检查lock是否释放的频率会越来越小。为什么会这样呢？因为检查lock值更消耗CPU，
                    而执行ngx_cpu_pause对于CPU的能耗来说是很省电的
                    */            
                for (i = 0; i < n; i++) {
                    /*
                        ngx_cpu_pause是在许多架构体系中专门为了自旋锁而提供的指令，它会告诉CPU现在处于自旋锁等待状悉，通常一些
                        CPU会将自己置于节能状态，降低功耗。注意，在执行ngx_cpu_pause后，当前进程没有“让出”正使用的处理器
                         */
                    __asm__ ("pause");
                }

                //检查锁是否被释放了，如果lock值为0且释放了锁后，就把它的值设为value，当前进程持有锁成功并返回
                if (*lock == 0 && __sync_bool_compare_and_swap(lock, 0, value)) {
                    return;
                }
            }
        }

        /*
          当前进程仍然处于可执行状态，但暂时“让出”处理器，使得处理器优先调度其他可执行状态的进程，这样，在进程被内核再次调度时，
          在for循环代码中可以期望其他进程释放锁。注意，不同的内核版本对于ngx_sched_yield系统调用的实现可能是不同的，但它们的目的都是暂时“让出”处理器
          */
        sched_yield();
    }
}