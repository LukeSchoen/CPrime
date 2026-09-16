// EXPECT_EXIT: 0
// A nested class template of the enclosing class spelled unqualified in a
// template argument list keeps its argument list and instantiates.
template <typename T> struct Probe { static const int size = sizeof(T); };

template <typename T> struct Outer {
  template <typename U> struct Inner {
    U first;
    U second;
  };
  typedef Probe<Inner<int> > probe;
  static int const nested_size = sizeof(Inner<char>);
};

template <typename T> struct Wrapper {
  template <typename U> struct Pair {
    static U const value = 1;
  };
  static int const size = sizeof(Pair<char>);
};

Outer<double>::Inner<int> value;
Wrapper<int>::Pair<int> wrapped;

int main() {
  value.first = 4;
  value.second = 5;
  if (value.first + value.second != 9)
    return 1;
  if (Outer<double>::nested_size != sizeof(Outer<double>::Inner<char>))
    return 2;
  if (Wrapper<int>::size != sizeof(Wrapper<int>::Pair<char>))
    return 3;
  return wrapped.value == 1 ? 0 : 4;
}
