constexpr int update() {
  int value = 3;
  int &alias = value;
  int *pointer = &value;
  int old = alias++;
  int current = ++*pointer;
  int before = (*pointer)--;
  int after = --alias;
  return old * 10000 + current * 1000 + before * 100 + after * 10 + value;
}
static_assert(update() == 35533);
int main() { return update() != 35533; }
