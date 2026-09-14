// EXPECT_COMPILE_ARGS: -std=c++17
struct Record { int value; };
struct ExplicitCopy {
  int value;
  ExplicitCopy(int v) : value(v) {}
  explicit ExplicitCopy(const ExplicitCopy &other) : value(other.value + 1) {}
};
int main() {
  Record record = {3};
  auto [copy](record);
  auto &[alias]{record};
  copy = 7;
  alias = 5;
  if (record.value != 5 || copy != 7) return 1;
  int values[2] = {11, 13};
  auto [a, b]{values};
  auto &[c, d](values);
  c = 17;
  ExplicitCopy source(19);
  auto [direct](source);
  auto [listed]{source};
  if (direct != 20 || listed != 20) return 2;
  return a != 11 || b != 13 || d != 13 || values[0] != 17;
}
