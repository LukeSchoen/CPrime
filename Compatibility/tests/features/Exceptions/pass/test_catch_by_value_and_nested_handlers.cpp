#include <exception>
static int copies;
static int destroyed;
struct Error {
  int value;
  explicit Error(int n) : value(n) {}
  Error(const Error &other) : value(other.value) { ++copies; }
  ~Error() { ++destroyed; }
};
int main() {
  Error original(83);
  try { throw original; }
  catch (Error caught) {
    if (caught.value != 83 || copies != 2 || destroyed) return 1;
    try { throw 19; }
    catch (int inner) { if (inner != 19 || std::uncaught_exceptions()) return 2; }
    try { throw; }
    catch (const Error &again) { if (again.value != 83 || copies != 2) return 3; }
  }
  if (destroyed != 2) return 4;
  try { throw original; }
  catch (Error) {}
  return copies == 4 && destroyed == 4 ? 0 : 5;
}
