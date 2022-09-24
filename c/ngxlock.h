#ifndef _LOCK_H_
#define _LOCK_H_
#include <sched.h>
typedef volatile unsigned long  ngx_atomic_t;
typedef unsigned long int ngx_uint_t;

void ngx_rwlock_wlock(ngx_atomic_t *lock);
void ngx_rwlock_rlock(ngx_atomic_t *lock);
void ngx_rwlock_unlock(ngx_atomic_t *lock);
void ngx_spinlock_lock(ngx_atomic_t *lock, long value, ngx_uint_t spin);
#define ngx_spinlock_trylock(lock)  (*(lock) == 0 && __sync_bool_compare_and_swap(lock, 0, 1))
#define ngx_spinlock_unlock(lock)    *(lock) = 0

#endif