#include <exception>
#include <stdlib.h>
static int copies;
static int observed_uncaught;
struct Error {
  Error() {}
  Error(const Error &) {
    ++copies;
    if (copies == 2) {
      observed_uncaught = std::uncaught_exceptions();
      throw 41;
    }
  }
};
static void terminated() {
  _Exit(copies == 2 && observed_uncaught == 1 && std::uncaught_exceptions() == 2 ? 0 : 1);
}
int main() {
  std::set_terminate(terminated);
  Error error;
  try { try { throw error; } catch (Error) {} }
  catch (...) { return 2; }
  return 3;
}
