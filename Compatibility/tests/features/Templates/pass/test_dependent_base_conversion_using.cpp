// EXPECT_EXIT: 0
struct Base
{
  operator Base *() const { return 0; }
};

template<class T>
struct Wrapper : T
{
  using T::operator T*;
  int check() const { return operator T*() != 0; }
};

int main()
{
  Wrapper<Base> wrapper;
  return wrapper.check();
}
