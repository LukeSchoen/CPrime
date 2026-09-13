// EXPECT_COMPILE_ONLY: 1
// The declarations alone must resolve: a conversion-operator member template
// named through a qualified-id is an addressable member even when its target
// class is incomplete and no definition of either conversion exists.

template <class U> struct Second;

template <class T> struct First
{
  int method ();

  template <class U> operator Second<U>();
  template <class U> operator First<U>();
};

template <class T> int First<T>::method () { return 0; }

struct B { };
struct D { };

void take ()
{
  First<B> (First<D>::*to_first)() = &First<D>::operator First<B>;
  Second<B> (First<D>::*to_second)() = &First<D>::operator Second<B>;
  (void)to_first;
  (void)to_second;
}
