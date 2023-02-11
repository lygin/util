#include "buffer_pool.h"
Page *BufferPool::GetPage(int page_id)
{
  if (page_id < 0 || page_id >= kMaxPages)
  {
    return nullptr;
  }
  if (pages_inmem_.find(page_id) == pages_inmem_.end())
  {
    Page *page = new Page();
    printf("new %d %p\n", page_id, page);
    page->SetPageId(page_id);
    pages_inmem_[page_id] = page;
    if(page_id * PAGE_SIZE < GetFileSize()) {
      disk_manager_->ReadPage(page_id, page->GetData());
    }
  }
  return pages_inmem_[page_id];
}
void BufferPool::FlushPage(int page_id)
{
  if (pages_inmem_.find(page_id) == pages_inmem_.end())
  {
    return;
  }
  Page *page = pages_inmem_[page_id];
  disk_manager_->WritePage(page_id, page->GetData());
}

BufferPool::BufferPool(const std::string &db_path) : disk_manager_(new DiskManager(db_path)),
    db_path_(db_path) {}
BufferPool::~BufferPool()
{
  FlushAll();
  for (auto iter = pages_inmem_.begin(); iter!= pages_inmem_.end(); iter++)
  {
    Page* page = iter->second;
    delete page;
  }
  delete disk_manager_;
}
void BufferPool::FlushAll()
{
  for (auto &p : pages_inmem_)
  {
    FlushPage(p.first);
  }
}
char *BufferPool::NewPage(int &new_page_id)
{
  int page_id = disk_manager_->AllocatePage();
  new_page_id = page_id;
  Page *page = GetPage(page_id);
  return page->GetData();
}

void BufferPool::FreePage(int page_id) {
  printf("delete page %d\n", page_id);
  delete pages_inmem_[page_id];
  pages_inmem_.erase(page_id);
}