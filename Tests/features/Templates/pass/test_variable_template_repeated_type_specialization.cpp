template<class A, class B> constexpr bool same = false;
template<class A> constexpr bool same<A, A> = true;
template<class T> constexpr int kind = 0;
template<class T> constexpr int kind<T *> = 1;
template<class T> constexpr int kind<const T *> = 2;
template<class T, int N> constexpr int extent = -1;
template<class T, int N> constexpr int extent<T[N], N> = N;
int main() {
  static_assert(same<int, int>);
  static_assert(same<int &, int &>);
  static_assert(!same<int, const int>);
  static_assert(kind<int> == 0);
  static_assert(kind<int *> == 1);
  static_assert(kind<const int *> == 2);
  static_assert(extent<int[3], 3> == 3);
  static_assert(extent<int[3], 4> == -1);
  return 0;
}
