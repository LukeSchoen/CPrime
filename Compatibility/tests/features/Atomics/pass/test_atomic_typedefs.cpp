// The standard atomic typedefs name the specializations of the primary
// template; boost's counted base stores its counts in a
// std::atomic_int_least32_t.
#include <atomic>
#include <cstdint>

void bump(std::atomic_int_least32_t *counter)
{
  counter->fetch_add(1, std::memory_order_relaxed);
}

int main()
{
  std::atomic_int_least32_t counter(0);
  bump(&counter);
  if (counter.load() != 1) return 1;
  std::atomic_size_t size(2);
  std::atomic_uint_least8_t small(3);
  if (size.load() != 2 || small.load() != 3) return 2;
  std::atomic_long wide(4);
  if (wide.load() != 4) return 3;
  return 0;
}
