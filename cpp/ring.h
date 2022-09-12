#pragma once

#include <cstdint>
#include <cstddef>

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif /* unlikely */
#ifndef compile_barrier
#define compile_barrier()         \
    do                            \
    {                             \
        asm volatile(""           \
                     :            \
                     :            \
                     : "memory"); \
    } while (0)
#endif

template <int is_sp, int is_sc, int prod_fixed, int cons_fixed>
class alignas(64) ring_impl
{
protected:
    const uint32_t capacity alignas(64);

private:
    char pad0 alignas(64);
    volatile uint32_t prod_head alignas(64){0};
    volatile uint32_t prod_tail{0};
    char pad1 alignas(64);
    volatile uint32_t cons_head alignas(64){0};
    volatile uint32_t cons_tail{0};
    char pad2 alignas(64);

protected:
    inline uint32_t move_prod_head(uint32_t n, uint32_t &old_head, uint32_t &new_head, uint32_t &free_entries)
    {
        uint32_t max = n;
        int success;
        do
        {
            n = max;
            old_head = prod_head;
            compile_barrier();
            free_entries = (capacity + cons_tail - old_head);
            if (unlikely(n > free_entries))
            {
                if (prod_fixed == 1)
                    n = 0;
                else
                    n = free_entries;
            }
            if (n == 0)
                return 0;
            new_head = old_head + n;
            if (is_sp)
                prod_head = new_head, success = 1;
            else
                success = __atomic_compare_exchange_n(&prod_head, &old_head, new_head, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED);
        } while (unlikely(success == 0));
        return n;
    }
    inline uint32_t move_cons_head(uint32_t n, uint32_t &old_head, uint32_t &new_head, uint32_t &entries)
    {
        uint32_t max = n;
        int success;
        do
        {
            n = max;
            old_head = cons_head;
            compile_barrier();
            entries = (prod_tail - old_head);
            if (n > entries)
            {
                if (cons_fixed == 1)
                    n = 0;
                else
                    n = entries;
            }
            if (unlikely(n == 0))
                return 0;
            new_head = old_head + n;
            if (is_sc)
            {
                cons_head = new_head;
                compile_barrier();
                success = 1;
            }
            else
                success = __atomic_compare_exchange_n(&cons_head, &old_head, new_head, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED);
        } while (unlikely(success == 0));
        return n;
    }

    inline void update_prod_tail(uint32_t old_val, uint32_t new_val)
    {
        compile_barrier();
        if (!is_sp)
            while (__atomic_load_n(&prod_tail, __ATOMIC_RELAXED) != old_val)
                __builtin_ia32_pause();
        prod_tail = new_val;
    }
    inline void update_cons_tail(uint32_t old_val, uint32_t new_val)
    {
        compile_barrier();
        if (!is_sc)
            while (__atomic_load_n(&cons_tail, __ATOMIC_RELAXED) != old_val)
                __builtin_ia32_pause();
        cons_tail = new_val;
    }

public:
    ring_impl(const uint32_t capacity = 255) : capacity(capacity)
    {
        if (capacity & (capacity + 1))
            throw "capacity should be 2^n-1";
    }
};