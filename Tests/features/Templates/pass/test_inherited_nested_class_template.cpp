// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
template<class T> struct Interface {
  template<class U> struct Rebind { typedef U* type; };
};
template<class T> struct Derived : Interface<T> {};
template<class T> struct Select {
  typedef typename T::template Rebind<int>::type type;
};
int main() {
  Select<Derived<char>>::type value = 0;
  return sizeof(value) != sizeof(int*);
}
