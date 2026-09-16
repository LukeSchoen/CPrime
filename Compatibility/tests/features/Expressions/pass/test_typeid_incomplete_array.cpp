// EXPECT_EXIT: 0
#include <typeinfo>

struct S;

int main()
{
  (void)typeid(S[3]);
  (void)typeid(S[]);
  (void)typeid(int[]);
  return 0;
}
