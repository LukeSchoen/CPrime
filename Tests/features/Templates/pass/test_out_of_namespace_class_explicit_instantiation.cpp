namespace N {
template<class T> int value()
{
  return int(sizeof(T));
}

template<class T> struct A
{
  int call() { return value<T>() + 1; }
};

template<class T> struct B
{
  T member;
};
}

template class N::A<int>;
template class ::N::B<long>;

int main()
{
  N::A<int> a;
  N::B<long> b;
  b.member = 7;
  return a.call() != 5 || b.member != 7;
}
