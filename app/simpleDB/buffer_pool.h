#pragma once
#include "header.h"
#include "disk_manager.h"
#include "coding.h"

const int kMaxPages = 1<<20;

class BufferPool {
  std::unordered_map<int, Page*> pages_inmem_;
  DiskManager* disk_manager_;
  const std::string db_path_;
  void FlushPage(int page_id);

  public:
  BufferPool(const std::string& db_path);
  ~BufferPool();
  Page *GetPage(int page_id);
  void FlushAll();
  void AppendDisk(const char *data, int len) {
    disk_manager_->Append(data, len);
  }
  void ReadDisk(char *data, int off, int len) {
    disk_manager_->Read(data, off, len);
  }
  int GetFileSize() {
    return disk_manager_->GetFileSize(db_path_);
  }
  uint32_t NextPageId() {
    return disk_manager_->NextPageId();
  }
  void SetNextPageId(uint32_t page_id) {
    disk_manager_->SetNextPageId(page_id);
  }
  char *NewPage(int &new_page_id);
  void FreePage(int page_id);
};