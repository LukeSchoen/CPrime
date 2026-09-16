// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
template<class T = int> struct Defaulted { T value; };
template<class... T> int count() { return sizeof...(T); }
template<class T> int nested() {
  Defaulted<Defaulted<>> value;
  value.value.value = 7;
  return value.value.value;
}
template<class T> using NestedAlias = Defaulted<Defaulted<>>;
int main() {
  Defaulted<> value;
  NestedAlias<double> alias;
  value.value = 3;
  alias.value.value = 5;
  if (value.value != 3 || alias.value.value != 5) return 1;
  if (count<>() != 0 || count<int, double>() != 2) return 2;
  return nested<char>() != 7;
}
