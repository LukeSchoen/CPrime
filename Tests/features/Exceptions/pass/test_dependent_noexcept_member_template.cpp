#include <exception>
#include <stdlib.h>
static int handled;
struct Calls {
  template<class T> static void check(T value) noexcept(sizeof(T) == sizeof(int)) { throw 97; }
};
static void terminated() { _Exit(handled == 97 && std::uncaught_exceptions() == 1 ? 0 : 1); }
int main() {
  std::set_terminate(terminated);
  try { Calls::check(1.5); } catch (int value) { handled = value; }
  try { Calls::check(2); } catch (...) { return 2; }
  return 3;
}
