#include <bits/stdc++.h>
using namespace std;

class Bucket
{
  int depth, size;
  std::map<int, string> values;

public:
  Bucket(int depth, int size)
  {
    this->depth = depth;
    this->size = size;
  }
  bool isFull()
  {
    if (values.size() == size)
      return 1;
    else
      return 0;
  }
  int insert(int key, string value)
  {
    std::map<int, string>::iterator it;
    it = values.find(key);
    if (it != values.end())
      return -1;
    if (isFull())
      return 1;
    values[key] = value;
    return 0;
  }
  int remove(int key)
  {
    std::map<int, string>::iterator it;
    it = values.find(key);
    if (it != values.end())
    {
      values.erase(it);
      return 0;
    }
    return -1;
  }
  int update(int key, string value)
  {
    std::map<int, string>::iterator it;
    it = values.find(key);
    if (it != values.end())
    {
      values[key] = value;
      return 0;
    }
    return -1;
  }
  int search(int key, string &ret)
  {
    std::map<int, string>::iterator it;
    it = values.find(key);
    if (it != values.end())
    {
      ret = it->second;
      return 0;
    }
    return -1;
  }
  bool isEmpty(void)
  {
    if (values.size() == 0)
      return 1;
    else
      return 0;
  }
  int getDepth(void)
  {
    return depth;
  }
  int increaseDepth(void)
  {
    depth++;
    return depth;
  }
  void clear(void)
  {
    values.clear();
  }
  std::map<int, string> copy(void)
  {
    std::map<int, string> temp(values.begin(), values.end());
    return temp;
  }
};

class Directory
{
  int global_depth, bucket_size;
  std::vector<Bucket *> buckets;

public:
  Directory(int depth, int bucket_size)
  {
    this->global_depth = depth;
    this->bucket_size = bucket_size;
    for (int i = 0; i < 1 << depth; i++)
    {
      buckets.push_back(new Bucket(depth, bucket_size));
    }
  }
  int hash(int key)
  {
    return key & ((1 << global_depth) - 1);
  }
  // like 10, 00 all points to bucket 0, because local_depth is 1
  int bucketIndex(int bucket_no, int depth)
  {
    return bucket_no & ((1 << depth) - 1);
  }
  void grow(void)
  {
    // grow 2 times bigger than current buckets
    for (int i = 0; i < 1 << global_depth; i++)
      buckets.push_back(buckets[i]); // by default, points to the original bucket
    global_depth++;
  }
  // split 1 bucket to 2 buckets
  void split(int bucket_no)
  {
    int local_depth, pair_index, index_diff, dir_size, i;
    map<int, string> temp;
    map<int, string>::iterator it;

    local_depth = buckets[bucket_no]->increaseDepth();
    // grow directories first
    if (local_depth > global_depth)
      grow();
    // alloc new bucket memory
    pair_index = bucketIndex(bucket_no, local_depth);
    buckets[pair_index] = new Bucket(local_depth, bucket_size);
    temp = buckets[bucket_no]->copy();
    buckets[bucket_no]->clear();

    index_diff = 1 << local_depth;
    dir_size = 1 << global_depth;
    // change coresponding bucket pointers
    for (i = pair_index - index_diff; i >= 0; i -= index_diff)
      buckets[i] = buckets[pair_index];
    for (i = pair_index + index_diff; i < dir_size; i += index_diff)
      buckets[i] = buckets[pair_index];
    // reinsert the total kvs, make it insert into 2 buckets
    for (it = temp.begin(); it != temp.end(); it++)
      insert((*it).first, (*it).second);
  }
  void insert(int key, string value)
  {
    int bucket_no = hash(key);
    int status = buckets[bucket_no]->insert(key, value);
    if (status == 1)
    {
      split(bucket_no);
      insert(key, value);
    }
  }
  void remove(int key, int mode)
  {
    int bucket_no = hash(key);
    buckets[bucket_no]->remove(key);
  }
  void update(int key, string value)
  {
    int bucket_no = hash(key);
    buckets[bucket_no]->update(key, value);
  }
  void search(int key, string& val)
  {
    int bucket_no = hash(key);
    buckets[bucket_no]->search(key, val);
  }
};