#include <cprime_exception.h>
static int log_value;
static int error_copies;
static int error_destructions;
struct Guard {
  int value;
  explicit Guard(int n) : value(n) {}
  ~Guard() { log_value = log_value * 10 + value; }
};
struct Error {
  int value;
  explicit Error(int n) : value(n) {}
  Error(const Error &other) : value(other.value) { ++error_copies; }
  ~Error() { ++error_destructions; }
};
static void fail(const Error &error) {
  Guard first(1);
  Guard second(2);
  throw error;
}
static void relay(const Error &error) {
  Guard outer(3);
  try { fail(error); }
  catch (const Error &caught) {
    if (caught.value != 47 || log_value != 21 || error_copies != 1) throw 1;
    if (__cpc_eh_uncaught_exceptions() != 0) throw 2;
    throw;
  }
}
int main() {
  Error original(47);
  try { relay(original); }
  catch (const Error &caught) {
    if (caught.value != 47 || log_value != 213 || error_copies != 1) return 1;
    if (error_destructions || __cpc_eh_uncaught_exceptions()) return 2;
  }
  catch (...) { return 3; }
  return error_destructions == 1 ? 0 : 4;
}
