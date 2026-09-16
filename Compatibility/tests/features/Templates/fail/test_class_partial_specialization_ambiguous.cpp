// EXPECT_COMPILE_FAIL: 1
template<class A, class B> struct Ambiguous {};
template<class T, class U> struct Ambiguous<T*, U> {};
template<class T, class U> struct Ambiguous<T, U*> {};
Ambiguous<int*, int*> value;
