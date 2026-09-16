template<class T> struct StripReference { typedef T type; };
template<class T> struct StripReference<T&> { typedef T type; };
template<class T> struct Outer {
  typedef typename StripReference<T>::type type;
  typedef typename StripReference<type&>::type again;
  type value;
};
template<class T> struct UsingOuter {
  using type = typename StripReference<T>::type;
  using again = typename StripReference<type&>::type;
  type value;
};
int main() {
  Outer<int&> value = {13};
  Outer<int&>::again copy = value.value;
  UsingOuter<double&> other = {4.5};
  UsingOuter<double&>::again other_copy = other.value;
  return copy != 13 || sizeof(Outer<int&>::type) != sizeof(int)
      || other_copy != 4.5 || sizeof(UsingOuter<double&>::type) != sizeof(double);
}
