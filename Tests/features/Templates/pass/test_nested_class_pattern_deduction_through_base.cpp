template<class T, class U> struct mp_list { };

template<class T> struct Wrap { struct type { }; };

struct Derived : mp_list<Wrap<int>::type, int>
               , mp_list<Wrap<long>::type, long> { };

template<class U>
U *select(mp_list<Wrap<int>::type, U> *)
{
  return static_cast<U *>(0);
}

int main()
{
  Derived value;
  int *selected = select(&value);
  return selected == 0 ? 0 : 1;
}
