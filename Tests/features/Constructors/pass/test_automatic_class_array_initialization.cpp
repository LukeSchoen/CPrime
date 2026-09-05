// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
static int constructed, destroyed, order, attempts, throw_attempt;
struct Object {
  const Object* self;
  int value;
  Object(int v = 3): self(this), value(v) {
    if (++attempts == throw_attempt) throw 17;
    ++constructed;
  }
  Object(const Object&) = delete;
  Object(Object&&) = delete;
  ~Object() { ++destroyed; order = order * 10 + value; }
};
int main() {
  {
    const Object values[] = {Object(2), Object(5)};
    if (sizeof(values) / sizeof(Object) != 2 || constructed != 2
        || values[0].value != 2 || values[1].value != 5
        || values[0].self != &values[0] || values[1].self != &values[1]) return 1;
  }
  if (destroyed != 2 || order != 52) return 2;
  constructed = destroyed = order = attempts = 0;
  {
    Object values[2][2] = {{Object(4)}, {Object(6), Object(7)}};
    if (constructed != 4 || values[0][0].value != 4 || values[0][1].value != 3
        || values[1][0].value != 6 || values[1][1].value != 7) return 3;
  }
  if (destroyed != 4 || order != 7634) return 4;
  constructed = destroyed = order = attempts = 0;
  throw_attempt = 3;
  try {
    Object values[4] = {Object(8), Object(9)};
    return 5;
  } catch (int value) {
    if (value != 17 || constructed != 2 || destroyed != 2 || order != 98) return 6;
  }
  throw_attempt = 0;
  return 0;
}
