constexpr int update() {
  int first = 1;
  int second = 2;
  int *pointer = &first;
  pointer = &second;
  *pointer = 8;
  return first * 10 + second;
}
static_assert(update() == 18);
int main() { return update() != 18; }
