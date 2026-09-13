template <class> class Base {
protected:
  typedef int Type;
};

template <class T> struct Derived : Base<T> {
  typedef typename Base<T>::Type Type;
  template <class Arg> static int f(Type value = Type()) { return value + sizeof(Arg); }
};

template int Derived<char>::f<int>(Type);

int main() { return Derived<char>::f<int>(3) == 7 ? 0 : 1; }
