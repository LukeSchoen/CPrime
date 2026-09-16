template<int N> struct Factor { static const long long num = N; static const long long den = 1; };
template<class T, int N> struct Value {
  typedef T rep;
  T value;
  explicit constexpr Value(const T &input) : value(input) {}
  constexpr T count() const { return value; }
};
template<class To, class T, int N> constexpr To convert(const Value<T,N> &from) {
  typedef typename To::rep target;
  typedef decltype(T() + target() + (long long)0) calculation;
  return To(static_cast<target>(static_cast<calculation>(from.count()) * Factor<1000>::num / Factor<1000>::den));
}
int main() { return convert<Value<long long,2>>(Value<long long,1>(123)).count() != 123000; }
