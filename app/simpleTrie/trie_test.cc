#include "trie.h"
#include <cassert>

int main() {
  Trie *trie = new Trie;
  trie->insert("apple");
  trie->insert("banana");
  trie->insert("orange");
  trie->insert("pear");
  assert(trie->find("apple") == true);
  assert(trie->find("banana") == true);
  assert(trie->find("orange") == true);
  assert(trie->find("pear") == true);
  assert(trie->find("xxxxx") == false);
  assert(trie->find("appl") == false);
  assert(trie->find("aplee") == false);
  assert(trie->find("APple") == true);
  assert(trie->find("BanANa") == true);
}