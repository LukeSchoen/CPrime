#include <exception>
#include <stdlib.h>
static int handled;
static void may_throw() noexcept(false) { throw 29; }
static void boundary() noexcept { throw 31; }
static void terminated() { _Exit(handled == 29 && std::uncaught_exceptions() == 1 ? 0 : 1); }
int main() {
  std::set_terminate(terminated);
  try { may_throw(); } catch (int value) { handled = value; }
  try { boundary(); } catch (...) { return 2; }
  return 3;
}
