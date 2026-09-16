// EXPECT_EXIT: 0
// A partial specialization of a member class template declared in the
// enclosing class template and named from the primary member's own body
// (`inner<T,int>::N`).  The injected class name with an explicit argument
// list still denotes the member template, so the specialization supplies N.
template <typename T>
struct outer
{
  template <typename T2, typename U>
  struct inner
  {
    static int value() { return inner<T, int>::N; }
  };

  template <typename U>
  struct inner<T, U>
  {
    static const int N = 1;
  };
};

int global_value = outer<int>::inner<double, int>::value();

int main()
{
  return global_value == 1 ? 0 : 1;
}
