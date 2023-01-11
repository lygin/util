#include <random>

/**
 * thread safty:
 * Either provide each thread with its own, thread-local engine, seeded on a thread-local seed
 * or synchronize access to the engine object.
 */
class Random
{
public:
  Random()
  {
    rand_.seed(dev_());
  }
  uint32_t rand32()
  {
    return rand_();
  }
  uint32_t Uniform(int n) { return rand32() % n;}
  // Randomly returns true ~"1/n" of the time, and false otherwise.
  bool OneIn(int n) { return (rand_() % n) == 0; }

protected:
  std::random_device dev_;
  std::mt19937 rand_;
};

class RandomRng : public Random
{
public:
  // return numbers of [lo, hi]
  explicit RandomRng(uint32_t lo, uint32_t hi) : dist_(lo, hi) {}
  uint32_t rand()
  {
    return dist_(rand_);
  }

private:
  std::uniform_int_distribution<std::mt19937::result_type> dist_;
};