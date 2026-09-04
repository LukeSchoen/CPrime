#include <cstddef>

int Check(const char*, const char* expected, const char* found,
          bool = true, bool = false)
{
  return expected == found ? 11 : -1;
}

int Check(const char*, bool expected, bool found,
          bool = true, bool = false)
{
  return expected == found ? 22 : -1;
}

template<class T>
int Check(const char*, T expected, T found, bool = true)
{
  return expected == found ? 33 : -1;
}

int main()
{
  const char* value = 0;
  return Check("null", NULL, value) == 11 ? 0 : 1;
}
