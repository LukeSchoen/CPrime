constexpr int run() {
  int first = 3, *pointer = &first, second = first + 1;
  int &alias = first, third = second + 1;
  int total = 0;
  for (int i = 0, end = second; i < end; ++i) total += i;
  *pointer += 1;
  return first + alias + second + third + total;
}
static_assert(run() == 23);
int main() { return run() != 23; }
