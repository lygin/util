#pragma once

#include <list>
#include <mutex> // NOLINT
#include <unordered_map>

#include "page.h"
#include "disk_manager.h"
#include "lrucache.h"
#include "arena.h"
#include "slice.h"

class BufferPoolManager
{
public:
    //pool_size是16的整数倍时，pool_size才精确
    BufferPoolManager(size_t pool_size, DiskManager *disk_manager): 
        pool_size_(pool_size), disk_manager_(disk_manager) {
        pages_ = new Page[pool_size_];
        cache_ = new ShardedLRUCache(pool_size, std::bind(&BufferPoolManager::DeletePageCallBack,this,std::placeholders::_1));
        for (size_t i = 0; i < pool_size_; ++i)
        {
            free_list_.emplace_back(&pages_[i]);
        }
    }
    ~BufferPoolManager() {
        delete[] pages_;
        delete cache_;
    }
    // remember release when not used anymore
    LRUEntry *FetchPage(page_id_t page_id) {
        Slice key((char*)&page_id, 4);
        auto ent = cache_->Lookup(key);
        //0.pool里有
        if(ent != nullptr) {
            return ent;//记得release
        }
        //1.pool里面没有
        Page* pg = nullptr;
        latch_.lock();
        //1.1 pool还有空位置
        if(free_list_.size()) {
            pg = free_list_.front();
            free_list_.pop_front();
        }
        latch_.unlock();
        //1.2 pool没有空位置，new后插入cache，因为这样会让pool膨胀，占用空间变多
        //如果插入的cache正好满了，会淘汰掉一个Page，这个Page会放入free_list
        //最终结果就是free_list不断变大
        //TODO：free_list满后，调用淘汰接口，淘汰一个LRUentry，放到free_list
        if(pg == nullptr) {
            pg = (Page*)arena_.AllocateAligned(sizeof(Page));
        }
        pg->page_id_ = page_id;
        //2.从dm中读上来
        disk_manager_->ReadPage(page_id, pg->GetData());
        auto ret = cache_->Insert(key, (void*)pg);
        return ret;
    }
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
