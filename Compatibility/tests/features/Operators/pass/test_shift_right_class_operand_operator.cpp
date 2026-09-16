// A builtin shift only combines the left operand's type, so `scalar >> class`
// must still reach an overloaded operator>> instead of converting the class
// operand as an integer.
struct Sink { int value; };

struct Source { int value; };

int operator>> (int lhs, Source const &rhs) { return lhs + rhs.value; }

int main() {
  Source source;
  source.value = 8;
  if ((32 >> source) != 40) return 1;
  return 0;
}
