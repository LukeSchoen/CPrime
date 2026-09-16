// EXPECT_COMPILE_ARGS: -Werror
#include <algorithm>

struct Number { int value; };
bool operator<(const Number& left, int right) { return left.value < right; }
bool operator<(int left, const Number& right) { return left < right.value; }

int main() {
  const int values[] = {1, 3, 3, 5};
  if (std::lower_bound(values, values + 4, 3) != values + 1) return 1;
  if (std::upper_bound(values, values + 4, 3) != values + 3) return 2;
  int mutable_values[] = {1, 3, 3, 5};
  const long long key = 3;
  if (std::lower_bound(mutable_values, mutable_values + 4, key) != mutable_values + 1)
    return 3;
  if (std::upper_bound(mutable_values, mutable_values + 4, key) != mutable_values + 3)
    return 4;
  const Number numbers[] = {{1}, {3}, {3}, {5}};
  if (std::lower_bound(numbers, numbers + 4, 3) != numbers + 1) return 5;
  if (std::upper_bound(numbers, numbers + 4, 3) != numbers + 3) return 6;
  return std::lower_bound(values, values, 7) != values;
}
