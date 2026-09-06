// EXPECT_COMPILE_ARGS: -Werror
template<class T> struct Size { T payload; static int value() { return sizeof(T); } };
template<class T> struct Nested {
  using Wide = Size<long long>;
  using Real = Size<long double>;
  using Pointer = Size<const unsigned long long*>;
};
struct Members {
  template<class T> int size() const { return sizeof(T); }
};
template<class T> int width() { return sizeof(T); }
int main() {
  Members member;
  if (member.size<long long>() != sizeof(long long)) return 1;
  if (width<long double>() != sizeof(long double)) return 2;
  if (Size<short int>::value() != sizeof(short)) return 3;
  if (Size<long int>::value() != sizeof(long)) return 4;
  if (Size<long long int>::value() != sizeof(long long)) return 5;
  if (Size<signed long long>::value() != sizeof(long long)) return 6;
  if (sizeof(typename Nested<int>::Wide) != sizeof(long long)) return 7;
  if (sizeof(typename Nested<int>::Real) != sizeof(long double)) return 8;
  if (sizeof(typename Nested<int>::Pointer) != sizeof(void*)) return 9;
  return 0;
}
