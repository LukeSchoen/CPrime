// EXPECT_COMPILE_ARGS: -std=c++17
// A pack expansion written in a function's return type must expand before the
// deduced arguments replace the pack spelling.
template<class... Ts> struct box { };

template<class... Ts>
box<Ts...> echo_box(Ts...)
{
  return box<Ts...>();
}

template<class... Ts>
box<Ts...> from_args(Ts &&...)
{
  return box<Ts...>();
}

template<class... Ts> int count_of(box<Ts...> &) { return (int)sizeof...(Ts); }

int main()
{
  box<int, char> by_value = echo_box(1, 'x');
  box<int, char> by_reference = from_args(1, 'x');
  return count_of(by_value) + count_of(by_reference) == 4 ? 0 : 1;
}
