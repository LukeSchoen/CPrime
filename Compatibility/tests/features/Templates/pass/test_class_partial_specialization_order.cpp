// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
template<class T> struct Forward { static const int value = 0; };
template<class T> struct Forward<T&> { static const int value = 1; };
template<class T> struct Forward<const T&> { static const int value = 2; };
template<class T> struct Forward<const volatile T&> { static const int value = 3; };

template<class T> struct Reverse { static const int value = 0; };
template<class T> struct Reverse<volatile T const&> { static const int value = 3; };
template<class T> struct Reverse<T const&> { static const int value = 2; };
template<class T> struct Reverse<T&> { static const int value = 1; };

template<class T> struct Pointers { static const int value = 0; };
template<class T> struct Pointers<T*> { static const int value = 1; };
template<class T> struct Pointers<const T*> { static const int value = 2; };
template<class T> struct Pointers<T**> { static const int value = 3; };

template<class A, class B> struct Repeated { static const int value = 0; };
template<class T> struct Repeated<T, T> { static const int value = 1; };
template<class T, class U> struct Repeated<T*, U> { static const int value = 2; };
// This final candidate is more specialized than both otherwise-incomparable matches.
template<class T> struct Repeated<T*, T*> { static const int value = 3; };

int main() {
  if (Forward<int&>::value != 1 || Forward<const int&>::value != 2
      || Forward<const volatile int&>::value != 3) return 1;
  if (Reverse<int&>::value != 1 || Reverse<const int&>::value != 2
      || Reverse<const volatile int&>::value != 3) return 2;
  if (Pointers<int*>::value != 1 || Pointers<const int*>::value != 2
      || Pointers<const int**>::value != 3) return 3;
  if (Repeated<int, int>::value != 1 || Repeated<int*, double>::value != 2
      || Repeated<int*, int*>::value != 3
      || Repeated<int*, double*>::value != 2) return 4;
  return 0;
}
