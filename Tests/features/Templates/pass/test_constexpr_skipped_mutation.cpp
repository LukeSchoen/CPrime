constexpr int change(int &value) { value += 3; return value; }
constexpr int run() {
  int value = 4;
  bool first = false && change(value);
  bool second = true || change(value);
  int size = sizeof(change(value));
  return value;
}
static_assert(run() == 4);
int main() { return run() != 4; }
