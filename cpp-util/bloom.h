#pragma once
#include <cstdint>
#include <cmath>
#include "murmur2.h"
__always_inline int test_bit_set_bit(uint8_t *buf, uint64_t bitno, bool is_set)
{
  unsigned long int byte = bitno >> 3;
  unsigned char c = buf[byte]; // expensive memory access
  unsigned char mask = 1 << (bitno % 8ul);
  // already set before
  if (c & mask)
  {
    return 1;
  }
  else
  {
    if (is_set)
    {
      buf[byte] = c | mask;
    }
    return 0;
  }
}
enum BloomStatus : int32_t {
  ERROR = -1,
  OK = 0,
  kNotExist = 0,
  kMayExist = 1
};
class Bloom
{
  uint32_t entries_;
  double error_rate_;
  uint64_t total_bits_; // bits = (entries * ln(error)) / ln(2)^2
  uint64_t total_bytes_;
  uint8_t hashes_; // hash_times = bpe * ln(2)
  double bpe_;     // bits per entry
  uint8_t *data_;
public:
  ~Bloom() {
    if(data_ != nullptr) free(data_);
  }
  void Reset() {
    if(data_ != nullptr) {
      memset(data_, 0, total_bytes_);
    }
  }
  // exp_entries: The expected number of entries which will be inserted.(at least 1000)
  // error_rate: 0.01 means error rate is 1%
  int Init(uint32_t exp_entries, double error_rate)
  {
    if (exp_entries < 1000 || error_rate <= 0 || error_rate >= 1)
    {
      return BloomStatus::ERROR;
    }
    entries_ = exp_entries;
    error_rate_ = error_rate;
    double denom = 0.480453013918201; // ln(2)^2
    bpe_ = -log(error_rate_) / denom; // bit per key
    double totalbits = (double)exp_entries * bpe_; // total bits
    total_bits_ = (uint64_t)totalbits;
    if (total_bits_ % 8)
    {
      total_bytes_ = (total_bits_ / 8) + 1; // total bytes
    }
    else
    {
      total_bytes_ = total_bits_ / 8;
    }
    hashes_ = (uint8_t)ceil(0.693147180559945 * bpe_); // hash_times = ln(2)*bpe
    data_ = (uint8_t *)calloc(total_bytes_, sizeof(uint8_t)); // data bytes
    if (data_ == nullptr)
    {
      return BloomStatus::ERROR;
    }
    return BloomStatus::OK;
  }
  int CheckAdd(const void *buffer, int len, int add)
  {
    unsigned char hits = 0;
    unsigned int a = Hash(buffer, len, 0x9747b28c);
    unsigned int b = Hash(buffer, len, a);
    unsigned long int x;
    unsigned long int i;
    //set all the hash bit
    for (i = 0; i < hashes_; i++)
    {
      x = (a + b * i) % total_bits_;// hashed position of bits
      if (test_bit_set_bit(data_, x, add))
      {
        hits++;
      }
      else if (!add)//add == 0
      {
        return BloomStatus::kNotExist;//key not exists
      }
    }

    if (hits == hashes_)
    {
      return BloomStatus::kMayExist; // key already exists
    }

    return BloomStatus::OK;
  }
  int Add(const void *buffer, int len) {
    return CheckAdd(buffer, len, 1);
  }
  int Check(const void *buffer, int len) {
    return CheckAdd(buffer, len, 0);
  }
};