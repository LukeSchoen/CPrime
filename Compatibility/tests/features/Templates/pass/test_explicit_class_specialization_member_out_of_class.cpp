// The primary template's out-of-class member definition arrives after an
// explicit class specialization is declared. It must not replay the primary
// body into A<char>; the specialization's own body stays authoritative.
template<class T> struct A {
  void set(int value);
};

static int stored;

struct C {
  template<class T> friend void A<T>::set(int);
};

template<> struct A<char> {
  void set(int value);
};

template<class T> void A<T>::set(int value)
{
  stored = value;
}

void A<char>::set(int value)
{
  stored = value * 3;
}

int main()
{
  A<char> narrow;
  narrow.set(4);
  int after_narrow = stored;
  A<int> wide;
  wide.set(5);
  return after_narrow != 12 || stored != 5;
}
