#pragma once
#include <cstdio>
#include "buffer_pool.h"
/**
 * @brief the degree of btree
 * key per node: [M-1, 2M-1]
 * child per node: [M, 2M]
 */
#define M 4
typedef struct btree_nodes
{
  int k[2 * M - 1];
  uint32_t childs[2 * M];//page id
  int key_num;
  bool is_leaf;
  int page_id;
} btree_node;

class BTree
{
protected:
  btree_node *roots;
  BufferPool *buffer_pool_;
  btree_node *btree_node_new();
  int btree_node_num; // btree_node counts
  btree_node* BTREE_NODE(uint32_t page_id) {
    if ((int)page_id < 0) return nullptr;
    return (btree_node*)buffer_pool_->GetPage(page_id)->GetData();
  }

  int btree_split_child(btree_node *parent, int pos, btree_node *child);

  void btree_insert_nonfull(btree_node *node, int target);

  void btree_merge_child(btree_node *root, int pos, btree_node *y, btree_node *z);

  void btree_delete_nonone(btree_node *root, int target);

  int btree_search_successor(btree_node *root);

  int btree_search_predecessor(btree_node *root);

  void btree_shift_to_left_child(btree_node *root, int pos, btree_node *y, btree_node *z);

  void btree_shift_to_right_child(btree_node *root, int pos, btree_node *y, btree_node *z);

  btree_node *btree_insert(btree_node *root, int target);

  btree_node *btree_delete(btree_node *root, int target);
  
public:
  BTree(BufferPool *buffer_pool);
  ~BTree(void);
  uint32_t root_id() {
    return roots->page_id;
  }

  void insert(int target)
  {
    roots = btree_insert(roots, target);
  };

  void del(int target)
  {
    roots = btree_delete(roots, target);
  };

  void NodeNum_print()
  {
    printf("%d\n", btree_node_num);
  };

  bool find(int key) {
    btree_node *p = roots;
    while(p) {
      //find the last key which key > insert key
      if(key < p->k[0]) {
        p = BTREE_NODE(p->childs[0]);
      } else {
        int i = 0;
        while(i < p->key_num && key >= p->k[i]) i++;
        if(i > 0 && p->k[i-1] == key) return true;
        p = BTREE_NODE(p->childs[i]);
      }
    }
    return false;
  }
};
