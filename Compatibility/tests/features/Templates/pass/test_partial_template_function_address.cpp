// EXPECT_EXIT: 0
template<class T, class U> int sum(T first, U second) { return first + second; }
int main() {
  int (*function)(double, int) = &sum<double>;
  int (*other)(int, long) = (int (*)(int, long)) &sum<int>;
  return function(3.0, 4) != 7 || other(5, 6L) != 11;
}
