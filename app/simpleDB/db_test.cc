#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "logging.h"
#include "db.h"

int main()
{
  {
  DB db("btree.db");

  int arr[] = {18, 31, 12, 10, 15, 48, 45, 47, 50, 52, 23, 30, 20};
  for (int i = 0; i < sizeof(arr) / sizeof(int); i++)
  {
    db.Insert(arr[i]);
  }
  for (int i = 0; i < sizeof(arr) / sizeof(int); i++) {
    LOG_ASSERT(db.Find(arr[i]) == true, "arr[%d] = %d", i, arr[i]);
  }
  assert(db.Find(0) == false);
  assert(db.Find(17) == false);
  assert(db.Find(100) == false);
  int todel[] = {15, 18, 23, 30, 31, 52, 50};

  for (int i = 0; i < sizeof(todel) / sizeof(int); i++)
  {
    db.Delete(todel[i]);
  }
  assert(db.Find(15) == false);
  assert(db.Find(30) == false);
  assert(db.Find(50) == false);
  db.Flush();
  }
  //recovery
  DB db("btree.db");
  int arr[] = {20, 12, 10, 47, 48};
  for (int i = 0; i < sizeof(arr) / sizeof(int); i++) {
    assert(db.Find(arr[i]) == true);
  }
  assert(db.Find(15) == false);
  assert(db.Find(30) == false);
  assert(db.Find(50) == false);
  return 0;
}