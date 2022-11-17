// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#include <cstdio>
#include <string>

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"

#include "leveldb/db.h"
#include "leveldb/options.h"
#include "leveldb/slice.h"

#include "timer.h"
#include "logging.h"

using rocksdb::DB;
using rocksdb::Options;
using rocksdb::PinnableSlice;
using rocksdb::ReadOptions;
using rocksdb::Status;
using rocksdb::WriteBatch;
using rocksdb::WriteOptions;


#if defined(OS_WIN)
std::string kDBPath = "C:\\Windows\\TEMP\\rocksdb_simple_example";
#else
std::string kDBPath = "/tmp/rocksdb_simple_example";
std::string klDBPath = "/tmp/leveldb_simple_example";
#endif

const int write_num = 1000'000;

int main()
{
  DB *db;
  leveldb::DB *ldb;
  Options options;
  leveldb::Options loptions;
  // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
  options.IncreaseParallelism();
  options.OptimizeLevelStyleCompaction();
  // create the DB if it's not already present
  options.create_if_missing = true;
  loptions.create_if_missing =true;

  leveldb::Status ls = leveldb::DB::Open(loptions, klDBPath, &ldb);
  // open DB
  Status s = DB::Open(options, kDBPath, &db);
  assert(s.ok());
  assert(ls.ok());

  // Put key-value
  s = db->Put(WriteOptions(), "key1", "value");
  assert(s.ok());
  std::string value;
  // get value
  s = db->Get(ReadOptions(), "key1", &value);
  assert(s.ok());
  assert(value == "value");

  // atomically apply a set of updates
  {
    WriteBatch batch;
    batch.Delete("key1");
    batch.Put("key2", value);
    s = db->Write(WriteOptions(), &batch);
  }

  s = db->Get(ReadOptions(), "key1", &value);
  assert(s.IsNotFound());

  db->Get(ReadOptions(), "key2", &value);
  assert(value == "value");

  {
    PinnableSlice pinnable_val;
    db->Get(ReadOptions(), db->DefaultColumnFamily(), "key2", &pinnable_val);
    assert(pinnable_val == "value");
  }

  {
    std::string string_val;
    // If it cannot pin the value, it copies the value to its internal buffer.
    // The intenral buffer could be set during construction.
    PinnableSlice pinnable_val(&string_val);
    db->Get(ReadOptions(), db->DefaultColumnFamily(), "key2", &pinnable_val);
    assert(pinnable_val == "value");
    // If the value is not pinned, the internal buffer must have the value.
    assert(pinnable_val.IsPinned() || string_val == "value");
  }

  PinnableSlice pinnable_val;
  s = db->Get(ReadOptions(), db->DefaultColumnFamily(), "key1", &pinnable_val);
  assert(s.IsNotFound());
  // Reset PinnableSlice after each use and before each reuse
  pinnable_val.Reset();
  db->Get(ReadOptions(), db->DefaultColumnFamily(), "key2", &pinnable_val);
  assert(pinnable_val == "value");
  pinnable_val.Reset();
  // The Slice pointed by pinnable_val is not valid after this point
  Timer t;
  for (int i = 0; i < write_num; ++i)
  {
    std::string value = "value" + std::to_string(i);
    std::string key = "key" + std::to_string(i);
    s = db->Put(WriteOptions(), key, value);
    assert(s.ok());
  }
  LOG_INFO("rocksdb write %d kv time :%.3lf us", write_num, t.GetDurationUs());

  t.Reset();
  for (int i = 0; i < write_num; ++i)
  {
    std::string value = "value" + std::to_string(i);
    std::string key = "key" + std::to_string(i);
    std::string get_value;
    s = db->Get(ReadOptions(), key, &get_value);
    assert(value == get_value);
  }
  LOG_INFO("rocksdb read %d kv time :%.3lf us", write_num, t.GetDurationUs());
  t.Reset();
  for(int i= 0; i < write_num; ++i) {
    std::string key = "key" + std::to_string(i);
    std::string value = "value" + std::to_string(i);
    ls = ldb->Put(leveldb::WriteOptions(), key, value);
    assert(ls.ok());
  }
  LOG_INFO("leveldb write %d kv time :%.3lf us", write_num, t.GetDurationUs());
  t.Reset();
  for(int i=0; i < write_num; ++i) {
    std::string key = "key" + std::to_string(i);
    std::string value = "value" + std::to_string(i);
    std::string get_value;
    ls = ldb->Get(leveldb::ReadOptions(), key, &get_value);
    assert(value == get_value);
  }
  LOG_INFO("leveldb read %d kv time :%.3lf us", write_num, t.GetDurationUs());
  delete db;

  return 0;
}