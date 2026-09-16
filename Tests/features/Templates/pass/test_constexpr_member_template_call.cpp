// EXPECT_COMPILE_ARGS: -std=c++17
// A constexpr member function template's instantiated body takes part in
// constant evaluation, and a `constexpr` specifier before its deduced auto
// return type does not stop that deduction.

template<class T>
struct Box
{
  T value;
  constexpr T read() const { return value; }
};

struct Factory
{
  template<class T>
  constexpr T twice(T value) const { return static_cast<T>(value + value); }

  template<class T>
  constexpr auto boxed(T value) const { return Box<T>{value}; }
};

int main()
{
  constexpr int doubled = Factory{}.twice(3);
  Factory factory;
  auto made = factory.boxed(4);
  static_assert(doubled == 6, "constexpr member template call");
  return doubled == 6 && made.read() == 4 ? 0 : 1;
}
