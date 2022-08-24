#include "slice.h"
enum Status {
    OK,
    NOT_FOUND
};
constexpr uint32_t BLOCK_SIZE = 1<<15; //32KB
constexpr uint32_t KEY_SIZE = 16;
constexpr uint32_t VALUE_SIZE = 128;
constexpr uint32_t SLOT_SIZE = KEY_SIZE+VALUE_SIZE;

class Block {
 public:
  Block() { memset(data_, 0, BLOCK_SIZE); }
  Block(const Block&) = delete;
  Block& operator=(const Block&) = delete;

  ~Block();
  
  size_t size() const { return size_; }
  void Add(const Slice& key, const Slice& value);
  Status Get(const Slice& key, Slice *value);


 private:

  char data_[BLOCK_SIZE];
  size_t size_{0};
  size_t write_offset_{0};  // Offset in data_ of restart array
};

void Block::Add(const Slice& key, const Slice& value) {
  assert(write_offset_ < BLOCK_SIZE);
  memcpy(data_+write_offset_, key.data(), key.size());
  write_offset_ += key.size();
  memcpy(data_+write_offset_, value.data(), value.size());
  write_offset_ += value.size();
  size_++;
}

Status Block::Get(const Slice& key, Slice *value) {
  for(int i=0; i<write_offset_; i+=SLOT_SIZE) {
    if(Slice(data_+i, KEY_SIZE) == key) {
      *value = Slice(data_+KEY_SIZE, VALUE_SIZE);
      return OK;
    }
  }
  return NOT_FOUND;
}