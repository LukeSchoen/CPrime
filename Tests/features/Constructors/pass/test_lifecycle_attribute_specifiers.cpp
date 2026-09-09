// EXPECT_EXIT: 0
int destroyed;
struct Attributed {
  int value;
  __attribute__((noinline)) Attributed() : value(7) {}
  inline virtual ~Attributed() { ++destroyed; }
};

struct OutOfLine {
  OutOfLine();
};
inline __attribute__((always_inline)) OutOfLine::OutOfLine() {}

int main() {
  {
    Attributed object;
    if (object.value != 7) return 1;
  }
  OutOfLine other;
  return destroyed != 1;
}
