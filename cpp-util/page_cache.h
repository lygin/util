#pragma once

#include <list>
#include <mutex>

#include "page.h"
#include "disk_manager.h"
#include "lrucache.h"
#include "arena.h"
#include "slice.h"

class PageCache
{
public:
    PageCache(size_t total_pages, DiskManager *disk_manager): 
        total_pages_(total_pages), disk_manager_(disk_manager) {
        pages_ = new Page[total_pages_];
        cache_ = new ShardedLRUCache(total_pages, [&](const Slice& k, void *val){
            DeletePageCallBack(k, val);
            printf("delete page %u\n", *(uint32_t*)k.data());
        });
        for (size_t i = 0; i < total_pages_; ++i)
        {
            free_list_.emplace_back(&pages_[i]);
        }
    }
    ~PageCache() {
        delete[] pages_;
        delete cache_;
    }
    bool WritePage(uint32_t lba, uint32_t off, const Slice& data) {
        LRUEntry *ent = FetchPage(lba);
        if (ent == nullptr) return false;
        Page* page = (Page*)ent->value;
        if(off + data.size() >= PAGE_SIZE) return false;
        memcpy(page->data_ + off, data.data(), data.size());
        ReleasePage(ent, true);
        return true;
    }
    char* ReadPage(uint32_t lba, uint32_t off) {
        LRUEntry *ent = FetchPage(lba);
        if (ent == nullptr) return nullptr;
        Page* page = (Page*)ent->value;
        if(off >= PAGE_SIZE) return nullptr;
        char* ret = page->data_ + off;
        ReleasePage(ent, false);
        return ret;
    }

    // remember release when not used anymore
    LRUEntry *FetchPage(uint32_t page_id) {
        Slice key((char*)&page_id, 4);
        auto ent = cache_->Lookup(key);
        if(ent != nullptr) {
            return ent;
        }
        Page* pg = nullptr;
        latch_.lock();
        // free_list 还有空位置
        if(free_list_.size()) {
            pg = free_list_.front();
            free_list_.pop_front();
        }
        latch_.unlock();
        // freelist 没有空位置
        // 如果插入的cache正好满了，会淘汰掉一个Page，这个Page会放入free_list
        // 最终结果就是free_list不断变大
        // TODO：free_list满后，调用淘汰接口，淘汰一个LRUentry，放到free_list
        if(pg == nullptr) {
            pg = (Page*)arena_.AllocateAligned(sizeof(Page));
        }
        pg->page_id_ = page_id;
        /* load first */
        disk_manager_->ReadPage(page_id, pg->GetData());
        auto ret = cache_->Insert(key, (void*)pg);
        return ret;
    }
    bool ReleasePage(LRUEntry *ent, bool is_dirty) {
        Page *pg = (Page*)ent->value;
        pg->is_dirty_ = is_dirty;
        cache_->Release(ent);
        return true;
    }
    bool FlushPage(Page* pg) {
        if(pg->IsDirty()) {
            disk_manager_->WritePage(pg->GetPageId(), pg->GetData());
            pg->is_dirty_ = false;
            return true;
        }
        return false;
    }
    //删除内存Page，但不会删除影响磁盘Page
    bool DeletePage(uint32_t page_id) {
        cache_->Erase(Slice((char*)&page_id, 4));
        return true;
    }
    void FlushAllPages() {
        for(size_t i=0; i<total_pages_; ++i) {
            FlushPage(&pages_[i]);
        }
    }
    inline Page *GetPages() { return pages_; }
    inline size_t PageInCacheNum() { return cache_->TotalElem(); }
private:
    void DeletePageCallBack(const Slice& k, void *val)
    {
        Page* pg = (Page*)val;
        if(pg->IsDirty()) {
            disk_manager_->WritePage(pg->GetPageId(), pg->GetData());
        }
        pg->ResetMemory();
        latch_.lock();
        free_list_.emplace_back(pg);
        latch_.unlock();
    }
    size_t total_pages_;
    Page *pages_;
    DiskManager *disk_manager_;
    ShardedLRUCache *cache_;
    std::list<Page *> free_list_;
    Arena arena_;
    // protects:free_list_
    std::mutex latch_;
};
