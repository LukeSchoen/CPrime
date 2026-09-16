namespace math {
template<class T> struct Value {
  T number;
  Value(T v = T()) : number(v) {}
  Value operator-(const Value& rhs) const { return Value(number - rhs.number); }
};
}
template<class A, class B> A difference(const math::Value<A>& a, const math::Value<B>& b) {
  math::Value<A> result = (math::Value<A>(A(b.number)) - a);
  return result.number;
}
float identity(int v) { return float(v); }
math::Value<int> make(int v) { return math::Value<int>(v); }
int main() {
  math::Value<float> a(2);
  math::Value<double> b(7);
  if (difference(a, b) != 5) return 1;
  if ((float(b.number) - a.number) != 5) return 2;
  if ((math::Value<int>()).number != 0) return 3;
  if ((math::Value<int>{9}).number != 9) return 4;
  if (sizeof(math::Value<int>(5)) != sizeof(math::Value<int>)) return 5;
  typedef int LocalInteger;
  float (*function)(LocalInteger) = (float(*)(LocalInteger))identity;
  if (function(11) != 11) return 6;
  math::Value<int> (*factory)(int) = (math::Value<int>(*)(int))make;
  if (factory(13).number != 13) return 7;
  int evaluations = 0;
  if ((math::Value<int>(++evaluations)).number != 1 || evaluations != 1) return 8;
  if (sizeof(math::Value<int>(++evaluations)) != sizeof(math::Value<int>)
      || evaluations != 1) return 9;
  return 0;
}
