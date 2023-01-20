#pragma once
#include <cstdio>
/**
 * @brief the degree of btree
 * key per node: [M-1, 2M-1]
 * child per node: [M, 2M]
 */
#define M 2

typedef struct btree_nodes
{
  int k[2 * M - 1];
  struct btree_nodes *p[2 * M];
  int key_num;
  bool is_leaf;
} btree_node;

class BTree
{
protected:
  btree_node *roots;
  btree_node *btree_node_new();
  int btree_node_num; // btree_node counts

  /**
   * @brief split child if key_num of key in child exceed 2M-1
   */
  int btree_split_child(btree_node *parent, int pos, btree_node *child);

  /**
   * @brief insert a value into btree
   * the key_num of key in node less than 2M-1
   */
  void btree_insert_nonfull(btree_node *node, int target);

  /**
   * @brief merge y, z and root->k[pos] to left
   * this appens while y and z both have M-1 keys
   */
  void btree_merge_child(btree_node *root, int pos, btree_node *y, btree_node *z);

  /**
   * @brief delete a vlue from btree
   * root has at least M keys
   */
  void btree_delete_nonone(btree_node *root, int target);

  /**
   * @brief find the leftmost value
   */
  int btree_search_successor(btree_node *root);

  /**
   * @brief find the rightmost value
   */
  int btree_search_predecessor(btree_node *root);

  /**
   * @brief shift a value from z to y
   */
  void btree_shift_to_left_child(btree_node *root, int pos, btree_node *y, btree_node *z);

  /**
   * @brief shift a value from z to y
   */
  void btree_shift_to_right_child(btree_node *root, int pos, btree_node *y, btree_node *z);

  btree_node *btree_insert(btree_node *root, int target);

  btree_node *btree_delete(btree_node *root, int target);

  void btree_level_display(btree_node *root);
  
public:
  BTree(void);
  ~BTree(void);

  void insert(int target)
  {
    roots = btree_insert(roots, target);
  };

  void level_display()
  {
    btree_level_display(roots);
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
        p = p->p[0];
      } else {
        int i = 0;
        while(i < p->key_num && key >= p->k[i]) i++;
        if(i > 0 && p->k[i-1] == key) return true;
        p = p->p[i];
      }
    }
    return false;
  }
};
