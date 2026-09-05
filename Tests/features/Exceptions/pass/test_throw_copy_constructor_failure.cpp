static int destruction_count;
struct Error {
  Error() {}
  Error(const Error &) { throw 53; }
  ~Error() { ++destruction_count; }
};
static void fail() { Error original; throw original; }
int main() {
  try { fail(); }
  catch (const Error &) { return 1; }
  catch (int value) { return value == 53 && destruction_count == 1 ? 0 : 2; }
  return 3;
}
