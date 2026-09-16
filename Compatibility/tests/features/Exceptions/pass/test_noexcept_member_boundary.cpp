#include <exception>
#include <stdlib.h>
static int handled;
struct Methods {
  void may_throw() const noexcept(false) { throw 61; }
  void boundary() const noexcept(sizeof(int) == 4) { throw 63; }
};
static void terminated() { _Exit(handled == 61 && std::uncaught_exceptions() == 1 ? 0 : 1); }
int main() {
  std::set_terminate(terminated);
  Methods value;
  try { value.may_throw(); } catch (int n) { handled = n; }
  try { value.boundary(); } catch (...) { return 2; }
  return 3;
}
