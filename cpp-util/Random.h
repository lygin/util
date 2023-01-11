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
    rand32_.seed(dev_());
    rand64_.seed(dev_());
  }
  uint32_t rand32()
  {
    return rand32_();
  }
  uint64_t rand64()
  {
    return rand64_();
  }

protected:
  std::random_device dev_;
  std::mt19937 rand32_;
  std::mt19937_64 rand64_;
};

class RandomRng : public Random
{
public:
  // return numbers of [lo, hi]
  explicit RandomRng(uint32_t lo, uint32_t hi) : dist32_(lo, hi) {}
  uint32_t rand()
  {
    return dist32_(rand32_);
  }

private:
  std::uniform_int_distribution<std::mt19937::result_type> dist32_;
};