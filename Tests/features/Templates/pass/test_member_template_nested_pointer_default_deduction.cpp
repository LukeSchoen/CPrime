// EXPECT_COMPILE_ONLY: 1

struct inner_default { };
struct outer_default { };

template<class T, class U = inner_default>
struct Inner { };

template<class T, class U = outer_default>
struct Outer { };

template<class T>
struct Owner
{
  template<class U>
  Owner(const Outer<Inner<T>*, U>&) { }
};

int main()
{
  Outer<Inner<int>*> value;
  Owner<int> owner(value);
  return 0;
}
