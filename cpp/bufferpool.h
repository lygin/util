#pragma once

#include <list>
#include <mutex> // NOLINT
#include <unordered_map>

#include "page.h"
#include "disk_manager.h"
#include "lrucache.h"
#include "arena.h"

// TODO：Arena并发控制
class BufferPoolManager
{
public:
    //pool_size是16的整数倍时，pool_size才精确
    BufferPoolManager(size_t pool_size, DiskManager *disk_manager);

    /**
     * Destroys an existing BufferPoolManager.
     */
    ~BufferPoolManager();

    LRUEntry *FetchPage(page_id_t page_id);
    bool ReleasePage(LRUEntry *ent, bool is_dirty);
    bool FlushPage(Page* pg);
    bool DeletePage(page_id_t page_id);
    void FlushAllPages();
    inline Page *GetPages() { return pages_; }
    inline size_t TotalElem() { return cache_->TotalElem(); }
private:
    void DeletePageCallBack(void *val)
    {
        Page* pg = reinterpret_cast<Page*>(val);
        if(pg->IsDirty()) {
            disk_manager_->WritePage(pg->GetPageId(), pg->GetData());
        }
        pg->ResetMemory();
        latch_.lock();
        free_list_.emplace_back(pg);
        latch_.unlock();
        LOG("page %u move to free list", pg->page_id_);
    }
    /** Number of pages in the buffer pool. */
    size_t pool_size_;
    /** Array of buffer pool pages. */
    Page *pages_;
    /** Pointer to the disk manager. */
    DiskManager *disk_manager_;
    /** Replacer to find unpinned pages for replacement. */
    ShardedLRUCache *cache_;
    /** List of free pages. */
    std::list<Page *> free_list_;
    Arena arena_;
    /**
     * latch protects:
     * - free_list_
     */
    std::mutex latch_;
};
