// A type-trait builtin call is a value, so it is usable as the default
// argument of a value template parameter.

template<bool Flag = __has_nothrow_assign (void)>
struct DefaultedBool {
  static const int value = Flag ? 1 : 2;
};

template<int Count = __is_pod (int) ? 4 : 5>
struct DefaultedCount {
  int data[Count];
};

int main ()
{
  DefaultedBool<> flag;
  DefaultedCount<> numbers;

  if (flag.value != 2)
    return 1;
  if (sizeof (numbers.data) != 4 * sizeof (int))
    return 2;
  return 0;
}
