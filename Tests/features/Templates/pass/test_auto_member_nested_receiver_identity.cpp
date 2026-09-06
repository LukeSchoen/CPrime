// EXPECT_COMPILE_ARGS: -Werror
template<class T> struct Value { T number; auto Length() const; };
template<class T> auto Value<T>::Length() const { return number * 2; }
template<class T> struct Outer {
  T prefix;
  Value<T> child;
  auto Area() const;
};
template<class T> auto Outer<T>::Area() const { return (T)(child.Length() * 3); }
int main() {
  Outer<double> value = {9, {2.5}};
  auto area = value.Area();
  if (area != 15) return 1;
  const Outer<double> another = {100, {1.25}};
  if (another.Area() != 7.5) return 2;
  if (value.child.Length() != 5 || value.Area() != 15) return 3;
  return 0;
}
