template<class T> struct Cv { static const int value = 0; };
template<class T> struct Cv<const T> { static const int value = 1; };
template<class T> struct Cv<volatile T> { static const int value = 2; };
template<class T> struct Cv<const volatile T> { static const int value = 3; };

template<class T> struct Reversed { static const int value = 0; };
template<class T> struct Reversed<T volatile const> { static const int value = 3; };
template<class T> struct Reversed<T volatile> { static const int value = 2; };
template<class T> struct Reversed<T const> { static const int value = 1; };

template<class T> struct Pointee { static const int value = 0; };
template<class T> struct Pointee<const T *> { static const int value = 1; };
template<class T> struct Pointee<volatile T *> { static const int value = 2; };
template<class T> struct Pointee<const volatile T *> { static const int value = 3; };

template<class A, class B> struct Repeated { static const int value = 0; };
template<class T> struct Repeated<T, T *> { static const int value = 1; };
template<class T> struct Repeated<const T, const T *> { static const int value = 2; };
template<class T> struct Repeated<volatile T, volatile T *> { static const int value = 3; };
template<class T> struct Repeated<const volatile T, const volatile T *> { static const int value = 4; };

int main() {
  if (Cv<int>::value != 0 || Cv<const int>::value != 1 || Cv<volatile int>::value != 2) return 1;
  if (Cv<const volatile long long>::value != 3) return 2;
  if (Reversed<const volatile long long>::value != 3) return 3;
  if (Pointee<const volatile int *>::value != 3) return 4;
  if (Repeated<const volatile int, const volatile int *>::value != 4) return 5;
  if (Repeated<const int, volatile int *>::value != 0) return 6;
  return 0;
}
