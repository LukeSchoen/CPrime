#include <exception>
#include <stdlib.h>
static int handled;
struct Allowed { ~Allowed() noexcept(false) { throw 101; } };
struct Boundary { ~Boundary() noexcept(true) { throw 103; } };
static void terminated() { _Exit(handled == 101 && std::uncaught_exceptions() == 1 ? 0 : 1); }
int main() {
  std::set_terminate(terminated);
  try { Allowed value; } catch (int code) { handled = code; }
  try { Boundary value; } catch (...) { return 2; }
  return 3;
}
