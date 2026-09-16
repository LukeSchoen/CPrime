// EXPECT_EXIT: 0
int add_three(int value) { return value + 3; }
int double_value(int value) { return value * 2; }

template<class Function>
int invoke_capture(Function function, int value)
{
  auto captured = [=]() mutable { return function(value); };
  auto copied = captured;
  return copied();
}

int main()
{
  int (*function)(int) = add_three;
  auto by_value = [function](int value) { return function(value); };
  auto by_reference = [&function](int value) { return function(value); };
  function = double_value;
  if (by_value(5) != 8 || by_reference(5) != 10) return 1;
  if (invoke_capture(add_three, 9) != 12) return 2;
  return invoke_capture(double_value, 9) != 18;
}
