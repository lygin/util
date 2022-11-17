#include "lz4.h"

int main() {
  int x = LZ4_compressBound(1);
  return 0;
}