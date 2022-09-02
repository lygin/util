#include "bufferpool.h"

#include <algorithm>

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager)
    : pool_size_(pool_size), disk_manager_(disk_manager)
{
    // We allocate a consecutive memory space for the buffer pool.
    pages_ = new Page[pool_size_];
    cache_ = new ShardedLRUCache(pool_size, std::bind(&BufferPoolManager::DeletePageCallBack,this,std::placeholders::_1));

    // Initially, every page is in the free list.
    for (size_t i = 0; i < pool_size_; ++i)
    {
        free_list_.emplace_back(&pages_[i]);
    }
}

BufferPoolManager::~BufferPoolManager()
{
    delete[] pages_;
    delete cache_;
}

LRUEntry *BufferPoolManager::FetchPage(page_id_t page_id)
{
    Slice key((char*)&page_id, 4);
    auto ent = cache_->Lookup(key);
    if(ent != nullptr) {
        LOG("cache hit");
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
    //但是有个问题就是 现在new的page，由于没有在pages[]中，所以会无法释放
    //TODO：free_list满后，调用淘汰接口，淘汰一个LRUentry，放到free_list
    if(pg == nullptr) {
        pg = (Page*)arena_.AllocateAligned(sizeof(Page));
    }
    pg->page_id_ = page_id;
    //2.从dm中读上来
    disk_manager_->ReadPage(page_id, pg->GetData());
    auto ret = cache_->Insert(key, (void*)pg);
    LOG("read from disk");
    return ret;//记得release
}
//FetchPage之后必须调用
bool BufferPoolManager::ReleasePage(LRUEntry *ent, bool is_dirty)
{
    Page *pg = (Page*)ent->value;
    pg->is_dirty_ = is_dirty;
    cache_->Release(ent);
    return true;
}

//下刷Page内容，不会删除内存Page
bool BufferPoolManager::FlushPage(Page* pg)
{
    if(pg->IsDirty()) {
        disk_manager_->WritePage(pg->GetPageId(), pg->GetData());
        pg->is_dirty_ = false;
        return true;
    }
    return false;
}
//删除内存Page，但不会删除影响Page
bool BufferPoolManager::DeletePage(page_id_t page_id)
{
    cache_->Erase(Slice((char*)&page_id, 4));
    return true;
}
//下刷Page内容，不会删除内存Page
void BufferPoolManager::FlushAllPages()
{
    for(size_t i=0; i<pool_size_; ++i) {
        FlushPage(&pages_[i]);
    }
}
