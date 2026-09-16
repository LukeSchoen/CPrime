// EXPECT_COMPILE_ONLY: 1
// Distinct array bounds create prior specializations of constrained member templates.
// Keep deduction proportional to calls, rather than replaying it for every old specialization.
template<class T> T&& instance();
template<class T> struct Buffer {
  Buffer() {}
  Buffer(const Buffer&) = default;
  template<long long N> Buffer(const T (&)[N]) {}
  template<class U> explicit Buffer(const Buffer<U>) {}
};
struct Text;
Text convert(int);
Text convert(double);
Text convert(const char*);
Text convert(const Buffer<char>&);
Text convert(const Buffer<Text>&);
Text convert(const Text&);
struct Text {
  Buffer<char> data;
  Text() {}
  Text(const Text&) = default;
  Text(const char*) {}
  template<class T, class = decltype(convert(instance<T>()))>
  explicit Text(const T& value) { *this = convert(value); }
  Text& operator+=(const Text&) { return *this; }
  template<class T> Text& operator+=(const T& value) { return *this += convert(value); }
};
Text operator+(const Text& a, const Text& b) { Text r=a; r+=b; return r; }
template<class T, class = decltype(convert(instance<T>()))>
Text operator+(const Text& a, const T& b) { Text r=a; r+=Text(b); return r; }
template<class T, class = decltype(convert(instance<T>()))>
Text operator+(const T& a, const Text& b) { Text r=Text(a); r+=b; return r; }
#define CASE(N) { char value[N] = {}; result += value + Text(a); }
#define GROUP(N) CASE(N) CASE(N+1) CASE(N+2) CASE(N+3) CASE(N+4) CASE(N+5) CASE(N+6) CASE(N+7)
Text sample(int a) {
  Text result;
  GROUP(1) GROUP(9) GROUP(17) GROUP(25) GROUP(33) GROUP(41) GROUP(49) GROUP(57)
  GROUP(65) GROUP(73) GROUP(81) GROUP(89) GROUP(97) GROUP(105) GROUP(113) GROUP(121)
  return result;
}