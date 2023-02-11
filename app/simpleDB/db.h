#pragma once
#include "btree.h"


class DB{
  public:
  DB(const std::string& db_path) : pool_(new BufferPool(db_path)), btree_(new BTree(pool_)) {}
  ~DB() {
    delete pool_;
    delete btree_;
  }
  void Insert(int data) {
    btree_->insert(data);
  }
  bool Find(int data) {
    return btree_->find(data);
  }
  void Delete(int data) {
    btree_->del(data);
  }
  void Flush() {
    pool_->FlushAll();
    char buf[4];
    EncodeFixed32(buf, btree_->root_id());
    pool_->AppendDisk(buf, 4);
    printf("Flush root pageid %d NextPageId %d\n", btree_->root_id(), pool_->NextPageId());
    EncodeFixed32(buf, pool_->NextPageId());
    pool_->AppendDisk(buf, 4);
  }
  private:
  BufferPool *pool_;
  BTree *btree_;
};