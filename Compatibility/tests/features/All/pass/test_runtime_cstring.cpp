// EXPECT_EXIT: 0

#include <cstring>

int main(void)
{
  const char *lhs = "cprime";
  const char *rhs = "cprime";
  return std::strlen(lhs) == 6 && std::memcmp(lhs, rhs, 7) == 0 ? 0 : 1;
}
