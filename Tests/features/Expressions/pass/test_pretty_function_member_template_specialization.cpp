// EXPECT_EXIT: 0
#include <string.h>

template<class T>
struct S3
{
  template<class U>
  static const char* h(U);
};

template<>
template<>
const char* S3<double>::h(int)
{
  return __PRETTY_FUNCTION__;
}

int main()
{
  return strcmp(S3<double>::h(7),
                "static const char* S3<T>::h(U) [with U = int; T = double]");
}
