#include <exception>
#include <stdlib.h>
static int raised;
static void raise_value() { raised = 41; throw 41; }
struct Boundary { void run() const throw() { raise_value(); } };
static void terminated() { _Exit(raised == 41 ? 0 : 1); }
int main() {
  std::set_terminate(terminated);
  Boundary boundary;
  try { boundary.run(); } catch (...) { return 2; }
  return 3;
}
