//
// Created by wyb on 17-6-27.
//

#include "ringbuffer.h"
#include <algorithm>
#include <cstring>
#include <cassert>

inline bool is_power_of_2(uint32_t num)
{
    return (num != 0 && (num & (num - 1)) == 0);
}

inline uint32_t hightest_one_bit(uint32_t num)
{
    num |= (num >> 1);
    num |= (num >> 2);
    num |= (num >> 4);
    num |= (num >> 8);
    num |= (num >> 16);
    return num - (num >> 1);
}

inline uint32_t roundup_pow_of_two(uint32_t num)
{
    return num > 1 ? hightest_one_bit((num - 1) << 1) : 1;
}

RingBuffer::RingBuffer(uint32_t size) : widx_(0), ridx_(0)
{
    _size = roundup_pow_of_two(size);
    _buffer = new uint8_t[_size];
}

RingBuffer::~RingBuffer()
{
    if (!_buffer)
        delete[] _buffer;
}

uint32_t RingBuffer::Put(const uint8_t *buffer, uint32_t len)
{
    len = std::min(len, _size - (widx_ - ridx_.load(std::memory_order_acq_rel)));
    // len = std::min(len, _size - (widx_ - ridx_));
    //  通用内存屏障，保证out读到正确的值，可能另外一个线程在修改out
    //  用smp_mb是因为上面读out，下面写_buffer
    //  smp_mb();

    uint32_t l = std::min(len, _size - (widx_ & (_size - 1)));
    memcpy(_buffer + (widx_ & (_size - 1)), buffer, l);
    memcpy(_buffer, buffer + l, len - l);
    // 写内存屏障，保证先写完_buffer，再修改in
    // smp_wmb();
    widx_.fetch_add(len, std::memory_order_release);
    // widx_ += len;

    return len;
}

uint32_t RingBuffer::Get(uint8_t *buffer, uint32_t len)
{
    if(size() == 0) return 0;
    len = std::min(len, widx_.load(std::memory_order_acquire) - ridx_);
    // 读内存屏障，保证读到正确的in，可能另外一个线程正在修改in
    // 用smp_rmb是因为上面读in，下面读_buffer
    // smp_rmb();

    uint32_t l = std::min(len, _size - (ridx_ & (_size - 1)));
    memcpy(buffer, _buffer + (ridx_ & (_size - 1)), l);
    memcpy(buffer + l, _buffer, len - l);
    // 通用内存屏障，保证先把buffer读出去，再修改out
    // 上面读_bufer，下面写out
    // smp_mb();
    ridx_.fetch_add(len, std::memory_order_acq_rel);
    // ridx_ += len;

    return len;
}