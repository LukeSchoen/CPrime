// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: cstring_names. <cstring> puts only memcmp and strlen into
// namespace std; strcmp and the rest of the string functions are missing.

#include <cstring>

int main()
{
  return std::strcmp("a", "b") < 0 ? 0 : 1;
}
