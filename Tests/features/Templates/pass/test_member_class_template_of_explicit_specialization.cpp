// EXPECT_EXIT: 0
// `template<> template<class U> struct A<int>::B { ... }` defines the member
// class template B of the enclosing class template A's explicit
// specialization A<int>, which the leading `template<>` header declares
// implicitly.  The members written there belong to A<int>::B<char>; the
// primary template's B is unaffected.
template<class T> struct A
{
  template<class U> struct B
  {
    int primary;
    int tag() { return 1; }
  };
};

template<> template<class U>
struct A<int>::B
{
  double specialized;
  int tag() { return 2; }
  template<class V> int combine(V value) const;
};

template<> template<> template<class V>
int A<int>::B<char>::combine(V value) const
{
  return (int)specialized * 10 + (int)value;
}

A<int>::B<char> specialized;
A<float>::B<char> generic;

int main()
{
  specialized.specialized = 7;
  if (sizeof(specialized) != sizeof(double)) return 1;
  if (sizeof(generic) != sizeof(int)) return 2;
  if (specialized.tag() != 2) return 3;
  if (generic.tag() != 1) return 4;
  if (specialized.combine(3) != 73) return 5;
  if (specialized.combine(4.5) != 74) return 6;
  return 0;
}
