// EXPECT_COMPILE_ARGS: -Werror
struct First { int first; };
struct Second {
  int second;
  int get() const { return second; }
};
struct Derived : First, Second {
  int last;
};
int from_rvalue(Second&& value) { return value.second; }
int main() {
  Derived value;
  value.first=11; value.second=17; value.last=23;
  Second& second=static_cast<Second&>(value);
  if (&second != static_cast<Second*>(&value) || second.get()!=17) return 1;
  second.second=29;
  if (value.first!=11 || value.second!=29 || value.last!=23) return 2;
  const Derived& immutable=value;
  const Second& viewed=static_cast<const Second&>(immutable);
  if (&viewed!=&second || viewed.get()!=29) return 3;
  static_cast<Second&>(value).second=31;
  Second replacement;
  replacement.second=29;
  static_cast<Second&>(value)=replacement;
  return from_rvalue(static_cast<Second&&>(value))!=29;
}
