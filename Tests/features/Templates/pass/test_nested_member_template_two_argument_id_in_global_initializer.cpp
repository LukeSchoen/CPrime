// EXPECT_EXIT: 0
// A member class template spelled with an explicit two-argument list in a
// namespace-scope initializer.  The initializer is saved for dynamic
// initialization, so its token scan has to keep the whole argument list
// instead of stopping at the comma between the arguments.
struct owner
{
  template <typename First, typename Second>
  struct inner
  {
    static int value() { return 7; }
  };
};

int global_value = owner::inner<char, short>::value();

int main()
{
  return global_value == 7 ? 0 : 1;
}
