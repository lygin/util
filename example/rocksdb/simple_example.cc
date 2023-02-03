#include <cstdio>
#include <string>
#include <future>

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"

#include "leveldb/db.h"
#include "leveldb/options.h"
#include "leveldb/slice.h"

#include "timer.h"
#include "logging.h"

using rocksdb::PinnableSlice;
const int kThreadNum = 4;

std::string kDBPath = "/tmp/rocksdb_simple_example";
std::string klDBPath = "/tmp/leveldb_simple_example";

const int write_num = 100'0000;

int main()
{
  rocksdb::DB *db;
  rocksdb::Options options;
  // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
  options.IncreaseParallelism();
  options.OptimizeLevelStyleCompaction();
  // create the DB if it's not already present
  options.create_if_missing = true;
  // open DB
  rocksdb::Status s = rocksdb::DB::Open(options, kDBPath, &db);
  assert(s.ok());

  // Put key-value
  s = db->Put(rocksdb::WriteOptions(), "key1", "value");
  assert(s.ok());
  std::string value;
  // get value
  s = db->Get(rocksdb::ReadOptions(), "key1", &value);
  assert(s.ok());
  assert(value == "value");

  // atomically apply a set of updates
  {
    rocksdb::WriteBatch batch;
    batch.Delete("key1");
    batch.Put("key2", value);
    s = db->Write(rocksdb::WriteOptions(), &batch);
  }

  s = db->Get(rocksdb::ReadOptions(), "key1", &value);
  assert(s.IsNotFound());

  db->Get(rocksdb::ReadOptions(), "key2", &value);
  assert(value == "value");

  {
    PinnableSlice pinnable_val;
    db->Get(rocksdb::ReadOptions(), db->DefaultColumnFamily(), "key2", &pinnable_val);
    assert(pinnable_val == "value");
  }

  {
    std::string string_val;
    // If it cannot pin the value, it copies the value to its internal buffer.
    // The intenral buffer could be set during construction.
    PinnableSlice pinnable_val(&string_val);
    db->Get(rocksdb::ReadOptions(), db->DefaultColumnFamily(), "key2", &pinnable_val);
    assert(pinnable_val == "value");
    // If the value is not pinned, the internal buffer must have the value.
    assert(pinnable_val.IsPinned() || string_val == "value");
  }

  PinnableSlice pinnable_val;
  s = db->Get(rocksdb::ReadOptions(), db->DefaultColumnFamily(), "key1", &pinnable_val);
  assert(s.IsNotFound());
  // Reset PinnableSlice after each use and before each reuse
  pinnable_val.Reset();
  db->Get(rocksdb::ReadOptions(), db->DefaultColumnFamily(), "key2", &pinnable_val);
  assert(pinnable_val == "value");
  pinnable_val.Reset();
  // The Slice pointed by pinnable_val is not valid after this point


  Timer t;
  std::future<void> wait[kThreadNum];
  for(int i=0; i<kThreadNum; i++) {
    wait[i] = std::async([&](int start){
      for (int i = 0; i < write_num; ++i)
      {
        std::string value = "value" + std::to_string(start+i);
        std::string key = "key" + std::to_string(start+i);
        s = db->Put(rocksdb::WriteOptions(), key, value);
        assert(s.ok());
      }
    }, i*write_num);
  }
  for(int i=0; i<kThreadNum; i++) {
    wait[i].wait();
  }
  LOG_INFO("rocksdb write iops: %f Mops", write_num*kThreadNum/t.GetDurationUs());

  t.Reset();
  for(int i=0; i<kThreadNum; i++) {
    wait[i] = std::async([&](int start){
      for (int i = 0; i < write_num; ++i)
      {
        std::string value = "value" + std::to_string(i+start);
        std::string key = "key" + std::to_string(i+start);
        std::string get_value;
        rocksdb::ReadOptions rdop;
        rdop.fill_cache = false;
        s = db->Get(rdop, key, &get_value);
        assert(value == get_value);
      }
    }, i*write_num);
  }
  for(int i=0; i<kThreadNum; i++) {
    wait[i].wait();
  }
  LOG_INFO("rocksdb read iops: %f Mops", write_num*kThreadNum/t.GetDurationUs());

  //----------------------------leveldb--------------------------------
  leveldb::DB *ldb;
  leveldb::Options loptions;
  loptions.create_if_missing =true;
  leveldb::Status ls = leveldb::DB::Open(loptions, klDBPath, &ldb);
  assert(ls.ok());

  t.Reset();
  for(int i=0; i<kThreadNum; i++) {
    wait[i] = std::async([&](int start){
      for(int i= 0; i < write_num; ++i) {
        std::string key = "key" + std::to_string(i+start);
        std::string value = "value" + std::to_string(i+start);
        ls = ldb->Put(leveldb::WriteOptions(), key, value);
        assert(ls.ok());
      }
    }, i*write_num);
  }
  for(int i=0; i<kThreadNum; i++) {
    wait[i].wait();
  }
  LOG_INFO("leveldb write iops: %f Mops",write_num*kThreadNum/t.GetDurationUs());
  t.Reset();
  for(int i=0; i<kThreadNum; i++) {
    wait[i] = std::async([&](int start){
      for(int i=0; i < write_num; ++i) {
        std::string key = "key" + std::to_string(i+start);
        std::string value = "value" + std::to_string(i+start);
        std::string get_value;
        ls = ldb->Get(leveldb::ReadOptions(), key, &get_value);
        assert(value == get_value);
      }
    }, i*write_num);
  }
  for(int i=0; i<kThreadNum; i++) {
    wait[i].wait();
  }
  LOG_INFO("leveldb read iops: %f Mops",write_num*kThreadNum/t.GetDurationUs());
  delete db;

  return 0;
}