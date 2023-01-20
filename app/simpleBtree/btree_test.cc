#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "btree.h"

int main()
{

  BTree bt;

  int arr[] = {18, 31, 12, 10, 15, 48, 45, 47, 50, 52, 23, 30, 20};
  for (int i = 0; i < sizeof(arr) / sizeof(int); i++)
  {
    bt.insert(arr[i]);
  }
  printf("display about B-Tree:\n");
  bt.level_display();
  printf("\n\n");
  for (int i = 0; i < sizeof(arr) / sizeof(int); i++) {
    assert(bt.find(arr[i]) == true);
  }
  assert(bt.find(0) == false);
  assert(bt.find(17) == false);
  assert(bt.find(100) == false);
  bt.NodeNum_print();
  printf("delete data...\n");
  int todel[] = {15, 18, 23, 30, 31, 52, 50};

  for (int i = 0; i < sizeof(todel) / sizeof(int); i++)
  {
    printf("after delete %d\n", todel[i]);
    bt.del(todel[i]);
  }

  bt.NodeNum_print();

  printf("\n\ndelete after data:\n");
  printf("display about B-Tree:\n");
  bt.level_display();
  return 0;
}