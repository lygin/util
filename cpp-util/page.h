#pragma once
#include <cstdint>
#include <cstring>
#include "rwlock.h"

constexpr int PAGE_SIZE = 4 << 10; // 4KB
constexpr int INVALID_PAGE_ID = -1;

using frame_id_t = int32_t; // frame id type
using page_id_t = int32_t;  // page id type
using lsn_t = int32_t;      // log sequence number type

class Page
{
public:
    friend class BufferPoolManager;
    Page() { ResetMemory(); }
    ~Page() = default;

    inline char *GetData() { return data_; }
    inline page_id_t GetPageId() { return page_id_; }

    inline bool IsDirty() { return is_dirty_; }

    inline void WLatch() { rwlatch_.WLock(); }
    inline void WUnlatch() { rwlatch_.Unlock(); }

    inline void RLatch() { rwlatch_.RLock(); }
    inline void RUnlatch() { rwlatch_.Unlock(); }

    inline void ResetMemory() { memset(data_, OFFSET_PAGE_START, PAGE_SIZE); }

protected:
    static_assert(sizeof(page_id_t) == 4);
    static_assert(sizeof(lsn_t) == 4);

    static constexpr size_t SIZE_PAGE_HEADER = 8;
    static constexpr size_t OFFSET_PAGE_START = 0;
    static constexpr size_t OFFSET_LSN = 4;

private:
    char data_[PAGE_SIZE]{0};
    page_id_t page_id_ = INVALID_PAGE_ID;
    bool is_dirty_ = false;
    Rwlock rwlatch_;
};
