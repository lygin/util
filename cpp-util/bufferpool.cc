#include "bufferpool.h"

#include <algorithm>

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
//删除内存Page，但不会删除影响磁盘Page
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
