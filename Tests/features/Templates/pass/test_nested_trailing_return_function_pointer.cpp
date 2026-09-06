// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
#include <utility>
template<class F> auto evaluate(F&& function)
  -> decltype(std::forward<F>(function)(2, 4))
{ return std::forward<F>(function)(2, 4); }
int plus(int left, int right) { return left + right; }
int main() { return evaluate(&plus) != 6; }
