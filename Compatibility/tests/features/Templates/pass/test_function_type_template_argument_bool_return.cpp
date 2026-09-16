// A function type whose return type is bool is a type-id, not a functional
// cast expression: `bool(int)` in a template argument list must stay a
// function type so a partial specialization pattern like `R(Args...)` and
// `std::function<bool(int)>` both match.

#include <functional>

template<class Signature> struct Callable;

template<class R, class... Args> struct Callable<R(Args...)>
{
  int value;
};

Callable<bool(int)> callable;
std::function<bool(int)> function;
std::function<bool()> nullaryFunction;

int main()
{
  callable.value = 1;
  if (callable.value != 1) return 1;
  return 0;
}
