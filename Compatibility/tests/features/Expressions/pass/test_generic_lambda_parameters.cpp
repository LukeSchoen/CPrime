struct Item
{
  int value;
};

auto makeAdder(int offset)
{
  return [offset](auto value) { return offset + value; };
}

template<class T>
int addOne(T value)
{
  auto add = [value](auto increment) { return value + increment; };
  return add(1);
}

int main()
{
  if (!__has_feature(cxx_generic_lambdas)) return 12;

  auto increment = [](auto value) { return value + 1; };
  if (increment(41) != 42) return 1;
  if (increment(0.5) != 1.5) return 2;

  auto add = [](auto left, auto right) { return left + right; };
  if (add(20, 22) != 42) return 3;
  if (add(0.5, 0.25) != 0.75) return 4;

  auto dereference = [](auto *pointer) { return *pointer; };
  int value = 42;
  if (dereference(&value) != 42) return 5;

  auto identity = [](const auto &item) { return item; };
  if (identity(42) != 42) return 6;

  auto field = [](auto item) { return item.value; };
  Item object{42};
  if (field(object) != 42) return 7;

  auto addOffset = makeAdder(7);
  if (addOffset(35) != 42) return 8;
  if (addOffset(35.5) != 42.5) return 15;

  int mutableValue = 7;
  auto mutate = [mutableValue](auto step) mutable {
    mutableValue += step;
    return mutableValue;
  };
  if (mutate(1) != 8 || mutate(1) != 9) return 9;

  auto packet = [](auto... values) { return 0; };
  if (packet(1, 2, 3) != 0) return 10;

  auto forward = [](auto &&item) -> decltype(auto) {
    return static_cast<decltype(item)>(item);
  };
  if (forward(value) != 42) return 11;

  auto noReturn = [](auto item) { (void)item; };
  noReturn(42);

  auto nested = [](auto value) {
    auto inner = [](auto innerValue) { return innerValue; };
    return inner(value);
  };
  if (nested(42) != 42) return 13;

  if (addOne(41) != 42) return 14;

  return 0;
}
