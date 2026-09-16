// Consolidated from similar standalone regressions; each cpc_case_N preserves one.

namespace cpc_case_0
{
template<typename T>
class Box
{
};

template<typename T>
int ElementSize(Box<T> *value)
{
  (void)value;
  return sizeof(T);
}

int run()
{
  Box<int> value;
  return ElementSize(&value) == sizeof(int) ? 0 : 1;
}
}

namespace cpc_case_1
{
template<class T>
T ReadValue(T *value)
{
  return *value;
}

int run()
{
  int value = 23;
  return ReadValue(&value) == 23 ? 0 : 1;
}
}

namespace cpc_case_2
{
template<class T>
void Exchange(T &left, T &right)
{
  T value = left;
  left = right;
  right = value;
}

int run()
{
  int a = 3;
  int b = 8;
  int *left = &a;
  int *right = &b;
  Exchange(left, right);
  return left == &b && right == &a ? 0 : 1;
}
}

int main()
{
  if (int code = cpc_case_0::run()) { return code; }
  if (int code = cpc_case_1::run()) { return code; }
  if (int code = cpc_case_2::run()) { return code; }
  return 0;
}
