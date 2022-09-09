#include <stdint.h>
#include <atomic>

/**
 * 单生产单消费线程安全，size固定
 */
class RingBuffer {
public:
  RingBuffer(uint32_t size);

  ~RingBuffer();

  uint32_t Put(const uint8_t *buffer, uint32_t len);

  uint32_t Get(uint8_t *buffer, uint32_t len);

  inline uint32_t capacity() { return _size; }
  inline uint32_t size() { return (widx_ - ridx_); }
  inline bool empty() { return widx_ <= ridx_; }

private:
  uint8_t *_buffer;
  uint32_t _size;
  std::atomic<uint32_t> widx_;
  std::atomic<uint32_t> ridx_;
};