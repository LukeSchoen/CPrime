// EXPECT_EXIT: 0

#include <utility>

int main(void)
{
  int lhs = 20;
  int rhs = 22;
  std::swap(lhs, rhs);
  return lhs == 22 && rhs == 20 ? 0 : 1;
}
