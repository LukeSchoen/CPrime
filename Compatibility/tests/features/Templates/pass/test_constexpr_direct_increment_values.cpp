constexpr int update() {
  int value = 3;
  int old = value++;
  int current = ++value;
  int before = value--;
  int after = --value;
  return old * 10000 + current * 1000 + before * 100 + after * 10 + value;
}
static_assert(update() == 35533);
int main() { return update() != 35533; }
