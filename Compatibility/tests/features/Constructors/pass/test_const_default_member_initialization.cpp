// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
int shared = 5;
struct Value {
  const long long invalid = -1;
  const int zero;
  const int braced = {7};
  const int values[3] = {2, 4};
  const int grid[2][2] = {{1}, {3, 4}};
  const char label[4] = "abc";
  int& reference = shared;
  const int& constant_reference = shared;
  const int* const pointer = &shared;
  Value() : zero() {}
};
int main() {
  Value value;
  if (value.invalid != -1 || value.zero != 0 || *value.pointer != 5) return 1;
  if (value.braced != 7 || value.values[0] != 2 || value.values[1] != 4
      || value.values[2] != 0 || value.grid[0][0] != 1 || value.grid[0][1] != 0
      || value.grid[1][0] != 3 || value.grid[1][1] != 4
      || value.label[0] != 'a' || value.label[3] != 0) return 2;
  value.reference = 9;
  return shared != 9 || value.constant_reference != 9;
}
