struct Root
{
  int f(int value) { return value + 1; }
};

template<class T> struct Wrapper : T {};

template<class T>
struct Mid : T
{
  using T::f;
  int g() { return f(41); }
};

int main()
{
  Mid<Wrapper<Root> > m;
  return m.g() != 42;
}
