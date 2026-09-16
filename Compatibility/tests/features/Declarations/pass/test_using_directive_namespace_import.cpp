/* A namespace-scope using-declaration has to see the members a namespace
   inherits through its own using-directives (transitively), and the imported
   name is a single overload set: `using foo::f;` brings in both `foo1::f` and
   `foo2::f` even though neither is a direct member of `foo`. */

namespace foo1
{
  template <class T> int f(T value) { return 1 + (int)value; }
}

namespace foo2
{
  template <class T> int f(T first, T second)
  {
    return 2 + (int)first + (int)second;
  }
}

namespace foo
{
  using namespace foo1;
  using namespace foo2;
}

using foo::f;

namespace inner
{
  int g(int value) { return value * 3; }
}

namespace outer
{
  using namespace inner;
}

using outer::g;

int main()
{
  if (f(1) != 2)
    return 1;
  if (f(1, 1) != 4)
    return 2;
  if (g(2) != 6)
    return 3;
  return 0;
}
