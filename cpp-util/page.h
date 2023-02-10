#pragma once
#include <cstdint>
#include <cstring>
#include "lock.h"

constexpr int PAGE_SIZE = 4 << 10; // 4KB
constexpr int INVALID_PAGE_ID = -1;

using lsn_t = int32_t;      // log sequence number type

class Page
{
public:
    // Allow PageCache access private members
    friend class PageCache;
    Page() { ResetMemory(); }
    ~Page() = default;

    inline char *GetData() { return data_; }
    inline uint32_t GetPageId() { return page_id_; }

    inline bool IsDirty() { return is_dirty_; }

    inline void WLatch() { rwlatch_.WriteLock(); }
    inline void WUnlatch() { rwlatch_.WriteUnlock(); }

    inline void RLatch() { rwlatch_.ReadLock(); }
    inline void RUnlatch() { rwlatch_.ReadUnlock(); }

    inline void ResetMemory() { memset(data_, 0, PAGE_SIZE); }

protected:
    static_assert(sizeof(uint32_t) == 4);
    static_assert(sizeof(lsn_t) == 4);

    static constexpr size_t SIZE_PAGE_HEADER = 8;
    static constexpr size_t OFFSET_PAGE_START = 0;
    static constexpr size_t OFFSET_LSN = 4;

private:
    char data_[PAGE_SIZE]{0};
    uint32_t page_id_ = INVALID_PAGE_ID;
    bool is_dirty_ = false;
    RWMutex rwlatch_;
};
