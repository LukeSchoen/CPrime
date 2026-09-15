constexpr int update() {
  int value = 1;
  ((value)) = 4;
  (value) += 2;
  int &alias = (value);
  ++(alias);
  return ((value));
}
static_assert(update() == 7);
int main() { return update() != 7; }
