// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
namespace {
template<int N> struct Matrix {
  int value;
  Matrix(int input) : value(input + N) {}
  template<class T> Matrix(const T &input, int) : value(input + N) {}
};

int evaluate() {
  Matrix<2> first(10);
  Matrix<3> second(20, 0);
  auto call = [first, second]() { return first.value + second.value; };
  return call();
}
}

int main() { return evaluate() == 35 ? 0 : 1; }
