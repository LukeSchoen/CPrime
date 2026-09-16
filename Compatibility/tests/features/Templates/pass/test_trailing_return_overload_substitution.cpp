// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
#include <utility>
template<class F, class... Args>
auto evaluate(F&& function, int, Args&&... args)
  -> decltype(std::forward<F>(function)(std::forward<Args>(args)...))
{ return std::forward<F>(function)(std::forward<Args>(args)...); }
template<class F, class Object, class... Args>
auto evaluate(F&& function, long, Object&& object, Args&&... args)
  -> decltype((std::forward<Object>(object).*function)(std::forward<Args>(args)...))
{ return (std::forward<Object>(object).*function)(std::forward<Args>(args)...); }
int add(int a, int b) { return a + b; }
struct Object { int value; int add(int n) { return value + n; } };
int main() {
  Object object = {8};
  if (evaluate(&add, 0, 2, 4) != 6) return 1;
  if (evaluate(&Object::add, 0, object, 3) != 11) return 2;
  return evaluate(&add, 0, 5, 7) != 12;
}
