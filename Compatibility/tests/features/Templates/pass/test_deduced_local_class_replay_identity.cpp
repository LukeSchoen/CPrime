#include <type_traits>

template<class T> auto make_value(T value)
{
  struct Outer {
    struct Inner { T value; };
    Inner inner;
    T get() const { return inner.value; }
  };
  {
    struct Outer { int unrelated; };
    Outer shadow = {9};
    if (shadow.unrelated != 9) value = T(0);
  }
  return Outer{{value}};
}

template<class T> auto make_lambda_value(T value)
{
  auto factory = [value]() {
    struct Result { T value; };
    return Result{value};
  };
  return factory();
}

namespace nested {
template<class T> auto make_anonymous(T value)
{
  struct { T value; } result = {value};
  return result;
}
}

int main()
{
  auto a = make_value(7);
  auto b = make_value(2.5f);
  auto again = make_value(11);
  static_assert(std::is_same<decltype(a), decltype(again)>::value, "same specialization");
  static_assert(!std::is_same<decltype(a), decltype(b)>::value, "distinct specialization");
  if (a.get() != 7 || b.get() != 2.5f || again.get() != 11) return 1;
  if (make_lambda_value(13).value != 13 || make_lambda_value(0.75f).value != 0.75f) return 2;
  if (nested::make_anonymous(17).value != 17 || nested::make_anonymous(1.5).value != 1.5) return 3;
  return 0;
}
