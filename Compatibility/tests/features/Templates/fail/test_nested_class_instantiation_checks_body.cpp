// EXPECT_COMPILE_FAIL: 1
template<class T> struct Outer {
  template<class U> struct Inner { int read() { return U::missing; } };
};
template struct Outer<int>::Inner<char>;
