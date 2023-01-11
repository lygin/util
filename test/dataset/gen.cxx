#include <random>
#include <cstdio>
class Random
{
public:
  Random()
  {
    rand_.seed(dev_());
  }
  uint64_t rand()
  {
    return rand_();
  }

protected:
  std::random_device dev_;
  std::mt19937_64 rand_;
};
const int N = 1000000;
int main() {
  FILE* fp = fopen("dataset.txt", "w+");
  Random rd;
  for(int i=0; i<N; ++i) {
    uint64_t num = rd.rand();
    fprintf(fp, "%lu\n", num);
  }
  fclose(fp);
  return 0;
}
