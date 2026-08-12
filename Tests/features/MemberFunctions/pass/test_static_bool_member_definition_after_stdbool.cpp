// EXPECT_EXIT: 0
class Tests
{
public:
  static bool RunAll();
};

#include <stdbool.h>

bool Tests::RunAll()
{
  return true;
}

int main()
{
  return Tests::RunAll() ? 0 : 1;
}
