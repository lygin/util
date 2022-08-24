#include "cache.h"
#include "block.h"

constexpr uint32_t CACHE_BLK_NUM = 1<<14;
class BlockCache {
public:
    BlockCache() : cache_(NewLRUCache(CACHE_BLK_NUM)) {}
    Status Get(const Slice& k, Slice *value) {
        LRUHandle* h = nullptr;
        Status s = FindBlock(k, &h);
        if(s == OK) {
            Block* blk= reinterpret_cast<Block*>(cache_->Value(h));
            s = blk->Get(k, value);
            cache_->Release(h);
        }
        return s;
    }
    Status Put(const Slice& k, const Slice& v) {
        LRUHandle* h = cache_->Insert()
        
    }
private:
    Status FindBlock(const Slice& k, LRUHandle** h) {
        *h = cache_->Lookup(k);
        if (*h == nullptr) {
            //此时应该RDMA读远程
            return NOT_FOUND;
        }
        return OK;
    }
    ShardedLRUCache* cache_;
    Block blks[CACHE_BLK_NUM];
};