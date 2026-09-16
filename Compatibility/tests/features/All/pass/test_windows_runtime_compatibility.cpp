#include <cstdio>
#include <cstring>
#include <thread>
#include <cmath>
#include <cstdlib>

extern "C" unsigned char _BitScanForward64(unsigned long *index,
                                             unsigned long long mask);
extern "C" long _InterlockedExchangeAdd(volatile long *target, long value);

static int compare_ints(const void *left, const void *right)
{
  int a = *(const int *)left;
  int b = *(const int *)right;
  return (a > b) - (a < b);
}

int main()
{
  unsigned long index = 99;
  if (_BitScanForward64(&index, 0) != 0)
    return 1;
  if (!_BitScanForward64(&index, 0x2000000000ULL))
    return 2;
  if (index != 37)
    return 9;

  volatile long value = 12;
  if (_InterlockedExchangeAdd(&value, 5) != 12 || value != 17)
    return 3;

  if (strnlen_s(nullptr, 8) != 0 || strnlen_s("abcdef", 3) != 3)
    return 4;
  if (strnlen_s("abcdef", 0) != 0 || strnlen_s("", 8) != 0 ||
      strnlen_s("abcdef", 8) != 6 || strnlen_s("ab\0cd", 5) != 2)
    return 11;

  if (std::thread::hardware_concurrency() == 0)
    return 5;
  std::this_thread::sleep_for(std::chrono::milliseconds(0));

  double e = std::exp(0.0);
  if (e != 1.0)
    return 6;

  int values[3] = { 3, 1, 2 };
  std::qsort(values, 3, sizeof(int), compare_ints);
  if (values[0] != 1 || values[1] != 2 || values[2] != 3)
    return 10;

  FILE *file = tmpfile();
  if (!file)
    return 7;
  if (_ftelli64(file) != 0)
    return 8;
  fclose(file);
  return 0;
}
