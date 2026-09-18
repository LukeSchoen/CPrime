/* A constructor's arguments are separate objects from the destination of the
   initialization being parsed, so a nested same-type temporary must not claim
   that destination.  When it did, the temporary lost its prvalue category and
   the enclosing call's reference parameter reported "rvalue reference cannot
   bind to an lvalue" - the shape `Str value = Path(Str()).Name();` hits. */
// EXPECT_EXIT: 0

struct Str {
  int value;
  Str() : value(0) {}
  Str(const Str &) : value(1) {}
  Str(Str &&) : value(2) {}
  Str &operator=(const Str &) { return *this; }
  Str &operator=(Str &&) { return *this; }
};

struct Path {
  Path(const Str &) {}
  Path(Str &&) {}
  Str Name() const { return Str(); }
};

Str from_nested_temporary() { return Path(Str()).Name(); }

int main() {
  Str value = Path(Str()).Name();
  if (value.value != 0) return 1;
  Str direct(Path(Str()).Name());
  if (direct.value != 0) return 2;
  Path path = Path(Str());
  (void)path;
  Str copied = from_nested_temporary();
  if (copied.value != 0) return 3;
  return 0;
}
