// EXPECT_COMPILE_ARGS: -Werror
int sum(int a, int b, int c) { return a + b + c; }
template<class... T> int squares(T... values) {
  return sum((values * values)...);
}
int category(int&) { return 1; }
int category(const int&) { return 2; }
int category(int&&) { return 4; }
int digits(int a, int b, int c) { return a * 100 + b * 10 + c; }
int none() { return 7; }
int one(int value) { return value; }
int pair(int a, int b) { return a * 10 + b; }
template<class... T> int empty(T... values) {
  return none((values + 1)...)
       + one(3, (values + 1)...)
       + one((values + 1)..., 5)
       + sizeof...(T) + sizeof...(values);
}
template<class... T> int increment(T*... values) {
  return sum(++*values...);
}
template<class A, class B> int converted(A a, B b) { return a + b; }
template<class... T> int calls(T... values) {
  return sum(converted<int, T>(values, 1)...);
}
template<class... T> int indexed(T... values) {
  return pair(values[1]...);
}
template<class... T> int forwarded(T&&... values) {
  return digits(category(static_cast<T&&>(values))...);
}
struct Member {
  template<class... T> int squares(T... values) {
    return sum((values * values)...);
  }
  template<class... T> int forwarded(T&&... values) {
    return digits(category(static_cast<T&&>(values))...);
  }
};
int main() {
  if (squares(2, 3, 4) != 29) return 1;
  int value = 3;
  const int fixed = 4;
  if (forwarded(value, fixed, 5) != 124) return 2;
  Member member;
  if (member.squares(2, 3, 4) != 29) return 3;
  if (member.forwarded(value, fixed, 5) != 124) return 4;
  if (empty() != 15) return 5;
  int a = 1, b = 2, c = 3;
  if (increment(&a, &b, &c) != 9 || a != 2 || b != 3 || c != 4) return 6;
  if (calls(2, 3L, 4) != 12) return 7;
  int first[] = {0, 2}, second[] = {0, 7};
  if (indexed(first, second) != 27) return 8;
  return 0;
}
