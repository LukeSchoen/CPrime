constexpr int update() {
  int value = 3;
  int &alias = value;
  int *pointer = &value;
  alias = 7;
  *pointer = 9;
  return value + alias + *pointer;
}
static_assert(update() == 27);
int main() { return update() != 27; }
