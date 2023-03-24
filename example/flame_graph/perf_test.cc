#include <bits/stdc++.h>
using namespace std;
const uint64_t N = 10'000'000'000;
void malloc_handle(uint64_t i) {
  auto p = malloc(8);
  free(p);
}
int main() {
  for(uint64_t i=0; i<N; ++i) {
    malloc_handle(i);
  }
  return 0;
}