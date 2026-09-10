// EXPECT_EXIT: 0

// `extern template` declares an explicit instantiation without requesting
// that instantiated members be emitted from this translation unit.  Class
// completion and inline member definitions remain available.
template<class T>
struct A
{
  void f() { }
};

extern template class A<int>;
extern template void A<char>::f();

int main()
{
  A<int> a;
  A<char> c;
  a.f();
  c.f();
  return 0;
}
