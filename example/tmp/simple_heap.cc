#include <bits/stdc++.h>
using namespace std;
const int N = 1000;
class Heap
{
  int *h;
  int sz;

private:
  void up(int idx)
  {
    while (idx > 1 && h[idx] < h[idx / 2])
    {
      swap(h[idx], h[idx / 2]);
      idx /= 2;
    }
  }
  void down(int idx)
  {
    while (idx * 2 <= sz)
    {
      int t = idx << 1;
      if (t + 1 <= sz && h[t + 1] < h[t])
        t++;
      if (h[t] < h[idx])
        swap(h[t], h[idx]);
      else
        break;
      idx = t;
    }
  }

public:
  Heap() : h((int *)malloc(N*sizeof(int))), sz(0) {}
  ~Heap() { free(h); }
  void insert(int x)
  {
    sz++;
    h[sz] = x;
    up(sz);
  }

  int get_min()
  {
    return h[1];
  }
  void pop_min()
  {
    swap(h[1], h[sz]);
    sz--;
    down(1);
  }
};

int main()
{
  Heap h;
  for(int i=0; i<N; ++i) {
    h.insert(rand()%2000);
  }
  for(int i=0; i<N; ++i) {
    printf("%d\n", h.get_min());
    h.pop_min();
  }
  return 0;
}