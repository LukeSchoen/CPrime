// EXPECT_EXIT: 0
#include <string.h>

template <typename T>
const char* foo(T, typename T::type c)
{
  (void)c;
  return __PRETTY_FUNCTION__;
}

struct x
{
  typedef int type;
};

int main()
{
  return strcmp(foo(x(), 3),
                "const char* foo(T, typename T::type) "
                "[with T = x; typename T::type = int]");
}
