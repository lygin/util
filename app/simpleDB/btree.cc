#include "btree.h"
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

btree_node *BTree::btree_node_new()
{
  int page_id;
  btree_node *node = (btree_node *)buffer_pool_->NewPage(page_id);
  if (NULL == node)
  {
    return NULL;
  }
  memset(node->k, 0, sizeof(int) * (2 * M - 1));
  memset(node->childs, -1, sizeof(uint32_t) * (2 * M));
  node->key_num = 0;
  node->is_leaf = true;
  node->page_id = page_id;

  return node;
}

int BTree::btree_split_child(btree_node *parent, int pos, btree_node *child)
{
  btree_node *new_child = btree_node_new();
  if (NULL == new_child)
  {
    return -1;
  }
  // 新节点的is_leaf与child相同，key的个数为M-1
  new_child->is_leaf = child->is_leaf;
  new_child->key_num = M - 1;

  // 将child后半部分的key拷贝给新节点
  for (int i = 0; i < M - 1; i++)
  {
    new_child->k[i] = child->k[i + M];
  }

  // 如果child不是叶子，还需要把指针拷过去，指针比节点多1
  if (false == new_child->is_leaf)
  {
    for (int i = 0; i < M; i++)
    {
      new_child->childs[i] = child->childs[i + M];
    }
  }

  child->key_num = M - 1;

  // child的中间节点需要插入parent的pos处，更新parent的key和pointer
  for (int i = parent->key_num; i > pos; i--)
  {
    parent->childs[i + 1] = parent->childs[i];
  }
  parent->childs[pos + 1] = new_child->page_id;

  for (int i = parent->key_num - 1; i >= pos; i--)
  {
    parent->k[i + 1] = parent->k[i];
  }
  parent->k[pos] = child->k[M - 1];

  parent->key_num += 1;
  return 0;
}

// 执行该操作时，node->key_num < 2M-1
void BTree::btree_insert_nonfull(btree_node *node, int target)
{
  if (1 == node->is_leaf)
  {
    // 如果在叶子中找到，直接插入
    int pos = node->key_num;
    while (pos >= 1 && target < node->k[pos - 1])
    {
      node->k[pos] = node->k[pos - 1];
      pos--;
    }

    node->k[pos] = target;
    node->key_num += 1;
    btree_node_num += 1;
  }
  else
  {
    // 沿着查找路径下降
    int pos = node->key_num;
    while (pos > 0 && target < node->k[pos - 1])
    {
      pos--;
    }
    Page *childp = buffer_pool_->GetPage(node->childs[pos]);
    btree_node *childnode = (btree_node *)childp->GetData();
    if (2 * M - 1 == childnode->key_num)
    {
      // 如果路径上有满节点则分裂
      btree_split_child(node, pos, childnode);
      if (target > node->k[pos])
      {
        pos++;
      }
    }

    btree_insert_nonfull(childnode, target);
  }
}

btree_node *BTree::btree_insert(btree_node *root, int target)
{
  if (NULL == root)
  {
    return NULL;
  }

  // 对根节点的特殊处理，如果根是满的，唯一使得树增高的情形
  if (2 * M - 1 == root->key_num)
  {
    btree_node *node = btree_node_new();
    if (NULL == node)
    {
      return root;
    }

    node->is_leaf = 0;
    node->childs[0] = root->page_id;
    btree_split_child(node, 0, root);
    btree_insert_nonfull(node, target);
    return node;
  }
  else
  {
    btree_insert_nonfull(root, target);
    return root;
  }
}

// 将y，root->k[pos], z合并到y节点，并释放z节点，y,z各有M-1个节点
void BTree::btree_merge_child(btree_node *root, int pos, btree_node *y, btree_node *z)
{
  // 将z中节点拷贝到y的后半部分
  y->key_num = 2 * M - 1;
  for (int i = M; i < 2 * M - 1; i++)
  {
    y->k[i] = z->k[i - M];
  }
  y->k[M - 1] = root->k[pos]; // k[pos]下降为y的中间节点

  // 如果z非叶子，需要拷贝pointer
  if (false == z->is_leaf)
  {
    for (int i = M; i < 2 * M; i++)
    {
      y->childs[i] = z->childs[i - M];
    }
  }

  // k[pos]下降到y中，更新key和pointer
  for (int j = pos + 1; j < root->key_num; j++)
  {
    root->k[j - 1] = root->k[j];
    root->childs[j] = root->childs[j + 1];
  }

  root->key_num -= 1;
  buffer_pool_->FreePage(z->page_id);
}

/************  删除分析   **************
*
在删除B树节点时，为了避免回溯，当遇到需要合并的节点时就立即执行合并，B树的删除算法如下：从root向叶子节点按照search规律遍历：
（1）  如果target在叶节点x中，则直接从x中删除target，情况（2）和（3）会保证当再叶子节点找到target时，肯定能借节点或合并成功而不会引起父节点的关键字个数少于t-1。
（2）  如果target在分支节点x中：
（a）  如果x的左分支节点y至少包含t个关键字，则找出y的最右的关键字prev，并替换target，并在y中递归删除prev。
（b）  如果x的右分支节点z至少包含t个关键字，则找出z的最左的关键字next，并替换target，并在z中递归删除next。
（c）  否则，如果y和z都只有t-1个关键字，则将targe与z合并到y中，使得y有2t-1个关键字，再从y中递归删除target。
（3）  如果关键字不在分支节点x中，则必然在x的某个分支节点p[i]中，如果p[i]节点只有t-1个关键字。
（a）  如果p[i-1]拥有至少t个关键字，则将x的某个关键字降至p[i]中，将p[i-1]的最大节点上升至x中。
（b）  如果p[i+1]拥有至少t个关键字，则将x个某个关键字降至p[i]中，将p[i+1]的最小关键字上升至x个。
（c）  如果p[i-1]与p[i+1]都拥有t-1个关键字，则将p[i]与其中一个兄弟合并，将x的一个关键字降至合并的节点中，成为中间关键字。
*
*/

btree_node *BTree::btree_delete(btree_node *root, int target)
{
  // 特殊处理，当根只有两个子女，两个子女的关键字个数都为M-1时，合并根与两个子女
  if (1 == root->key_num)
  {
    btree_node *y = BTREE_NODE(root->childs[0]);
    btree_node *z = BTREE_NODE(root->childs[1]);
    if (NULL != y && NULL != z &&
        M - 1 == y->key_num && M - 1 == z->key_num)
    {
      btree_merge_child(root, 0, y, z);
      buffer_pool_->FreePage(root->page_id);
      btree_delete_nonone(y, target);
      return y;
    }
    else
    {
      btree_delete_nonone(root, target);
      return root;
    }
  }
  else
  {
    btree_delete_nonone(root, target);
    return root;
  }
}

// root至少有个t个关键字，保证不会回溯
void BTree::btree_delete_nonone(btree_node *root, int target)
{
  if (true == root->is_leaf)
  {
    // 如果在叶子节点，直接删除
    int i = 0;
    while (i < root->key_num && target > root->k[i])
      i++;
    if (target == root->k[i])
    {
      for (int j = i + 1; j < 2 * M - 1; j++)
      {
        root->k[j - 1] = root->k[j];
      }
      root->key_num -= 1;

      btree_node_num -= 1;
    }
    else
    {
      printf("target not found\n");
    }
  }
  else
  {
    int i = 0;
    btree_node *y = NULL, *z = NULL;
    while (i < root->key_num && target > root->k[i])
      i++;
    if (i < root->key_num && target == root->k[i])
    {
      // 如果在分支节点找到target
      y = BTREE_NODE(root->childs[i]);
      z = BTREE_NODE(root->childs[i + 1]);
      if (y->key_num > M - 1)
      {
        // 如果左分支关键字多于M-1，则找到左分支的最右节点prev，替换target
        // 并在左分支中递归删除prev,情况2（a)
        int pre = btree_search_predecessor(y);
        root->k[i] = pre;
        btree_delete_nonone(y, pre);
      }
      else if (z->key_num > M - 1)
      {
        // 如果右分支关键字多于M-1，则找到右分支的最左节点next，替换target
        // 并在右分支中递归删除next,情况2(b)
        int next = btree_search_successor(z);
        root->k[i] = next;
        btree_delete_nonone(z, next);
      }
      else
      {
        // 两个分支节点数都为M-1，则合并至y，并在y中递归删除target,情况2(c)
        btree_merge_child(root, i, y, z);
        btree_delete(y, target);
      }
    }
    else
    {
      // 在分支没有找到，肯定在分支的子节点中
      y = BTREE_NODE(root->childs[i]);
      if (i < root->key_num)
      {
        z = BTREE_NODE(root->childs[i + 1]);
      }
      btree_node *p = NULL;
      if (i > 0)
      {
        p = BTREE_NODE(root->childs[i - 1]);
      }

      if (y->key_num == M - 1)
      {
        if (i > 0 && p->key_num > M - 1)
        {
          // 左邻接节点关键字个数大于M-1
          // 情况3(a)
          btree_shift_to_right_child(root, i - 1, p, y);
        }
        else if (i < root->key_num && z->key_num > M - 1)
        {
          // 右邻接节点关键字个数大于M-1
          // 情况3(b)
          btree_shift_to_left_child(root, i, y, z);
        }
        else if (i > 0)
        {
          // 情况3（c)
          btree_merge_child(root, i - 1, p, y); // note
          y = p;
        }
        else
        {
          // 情况3(c)
          btree_merge_child(root, i, y, z);
        }
        btree_delete_nonone(y, target);
      }
      else
      {
        btree_delete_nonone(y, target);
      }
    }
  }
}

// 寻找rightmost，以root为根的最大关键字
int BTree::btree_search_predecessor(btree_node *root)
{
  btree_node *y = root;
  while (false == y->is_leaf)
  {
    y = BTREE_NODE(y->childs[y->key_num]);
  }
  return y->k[y->key_num - 1];
}

// 寻找leftmost，以root为根的最小关键字
int BTree::btree_search_successor(btree_node *root)
{
  btree_node *z = root;
  while (false == z->is_leaf)
  {
    z = BTREE_NODE(z->childs[0]);
  }
  return z->k[0];
}

// z向y借节点，将root->k[pos]下降至z，将y的最大关键字上升至root的pos处
void BTree::btree_shift_to_right_child(btree_node *root, int pos,
                                       btree_node *y, btree_node *z)
{
  z->key_num += 1;
  for (int i = z->key_num - 1; i > 0; i--)
  {
    z->k[i] = z->k[i - 1];
  }
  z->k[0] = root->k[pos];
  root->k[pos] = y->k[y->key_num - 1];

  if (false == z->is_leaf)
  {
    for (int i = z->key_num; i > 0; i--)
    {
      z->childs[i] = z->childs[i - 1];
    }
    z->childs[0] = y->childs[y->key_num];
  }

  y->key_num -= 1;
}

// y向z借节点，将root->k[pos]下降至y，将z的最小关键字上升至root的pos处
void BTree::btree_shift_to_left_child(btree_node *root, int pos,
                                      btree_node *y, btree_node *z)
{
  y->key_num += 1;
  y->k[y->key_num - 1] = root->k[pos];
  root->k[pos] = z->k[0];

  for (int j = 1; j < z->key_num; j++)
  {
    z->k[j - 1] = z->k[j];
  }

  if (false == z->is_leaf)
  {
    y->childs[y->key_num] = z->childs[0];
    for (int j = 1; j <= z->key_num; j++)
    {
      z->childs[j - 1] = z->childs[j];
    }
  }

  z->key_num -= 1;
}


BTree::BTree(BufferPool *buffer_pool): buffer_pool_(buffer_pool)
{
  btree_node_num = 0;
  int file_size = buffer_pool_->GetFileSize();
  if(file_size<=0) {
    roots = btree_node_new();
  } else {
    char buf[4];
    buffer_pool_->ReadDisk(buf, file_size-8, 4);
    uint32_t root_id = DecodeFixed32(buf);
    buffer_pool_->ReadDisk(buf, file_size-4, 4);
    uint32_t next_page_id = DecodeFixed32(buf);
    buffer_pool->SetNextPageId(next_page_id);
    printf("---------recovery--------\n");
    printf("root_page_id = %d next_page_id = %d\n", root_id, next_page_id);
    roots = BTREE_NODE(root_id);
  }
  
}

BTree::~BTree(void)
{
}
