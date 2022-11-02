#pragma once
#include <cstdint>
#include <cstring>
#include "rwlock.h"

#define DEBUG
#ifdef DEBUG
#define LOG(frm, argc...)                         \
    {                                             \
        printf("[%s : %d] ", __func__, __LINE__); \
        printf(frm, ##argc);                      \
        printf("\n");                             \
    }
#else
#define LOG(frm, argc...)
#endif

constexpr int PAGE_SIZE = 4 << 10; // 4KB
constexpr int INVALID_PAGE_ID = -1;

using frame_id_t = int32_t; // frame id type
using page_id_t = int32_t;  // page id type
using lsn_t = int32_t;      // log sequence number type

class Page
{
public:
    friend class BufferPoolManager;
    /** Constructor. Zeros out the page data. */
    Page() { ResetMemory(); }

    /** Default destructor. */
    ~Page() = default;

    /** @return the actual data contained within this page */
    inline char *GetData() { return data_; }

    /** @return the page id of this page */
    inline page_id_t GetPageId() { return page_id_; }

    /** @return true if the page in memory has been modified from the page on disk, false otherwise */
    inline bool IsDirty() { return is_dirty_; }

    /** Acquire the page write latch. */
    inline void WLatch() { rwlatch_.WLock(); }

    /** Release the page write latch. */
    inline void WUnlatch() { rwlatch_.WUnlock(); }

    /** Acquire the page read latch. */
    inline void RLatch() { rwlatch_.RLock(); }

    /** Release the page read latch. */
    inline void RUnlatch() { rwlatch_.RUnlock(); }

    inline void ResetMemory() { memset(data_, OFFSET_PAGE_START, PAGE_SIZE); }

protected:
    static_assert(sizeof(page_id_t) == 4);
    static_assert(sizeof(lsn_t) == 4);

    static constexpr size_t SIZE_PAGE_HEADER = 8;
    static constexpr size_t OFFSET_PAGE_START = 0;
    static constexpr size_t OFFSET_LSN = 4;

private:
    /** Zeroes out the data that is held within the page. */

    /** The actual data that is stored within a page. */
    char data_[PAGE_SIZE]{};
    /** The ID of this page. */
    page_id_t page_id_ = INVALID_PAGE_ID;
    /** True if the page is dirty, i.e. it is different from its corresponding page on disk. */
    bool is_dirty_ = false;
    /** Page latch. */
    RWLock rwlatch_;
};
