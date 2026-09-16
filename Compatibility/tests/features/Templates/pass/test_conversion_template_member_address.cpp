// A qualified conversion-operator name carries its member template argument
// list inside the conversion-type-id (`&First<B>::operator First<int>`), so the
// ordinary `<...>` member-argument path never sees it.  The contextual
// member-pointer type accepts only the named specialization, so forming these
// addresses pins which one the qualified name selected.

template <class U> struct Second;

template <class T> struct First
{
  int value;

  First () : value(0) { }
  First (int seed) : value(seed) { }

  int stored () { return value; }

  template <class U> operator Second<U>() {
    Second<U> result;
    result.value = value + (int)sizeof(U);
    return result;
  }
  template <class U> operator First<U>() {
    return First<U>(value + (int)sizeof(U));
  }
};

template <class U> struct Second
{
  int value;
  Second () : value(0) { }
};

struct B { };

int main ()
{
  First<char> (First<B>::*to_first)() = &First<B>::operator First<char>;
  Second<int> (First<B>::*to_second)() = &First<B>::operator Second<int>;
  if (!(bool)to_first) return 1;
  if (!(bool)to_second) return 2;

  First<B> source(4);
  First<char> converted = source.operator First<char>();
  Second<int> other = source.operator Second<int>();
  if (converted.value != 4 + (int)sizeof(char)) return 3;
  if (other.value != 4 + (int)sizeof(int)) return 4;

  // A plain class keeps the same spelling for its non-template conversion.
  int (First<B>::*stored)() = &First<B>::stored;
  if ((source.*stored)() != 4) return 5;
  return 0;
}
