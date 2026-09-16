#include <cstdlib>
#include <new>
int compare(const void *left, const void *right) {
  return *(const int *)left - *(const int *)right;
}
int main() {
  std::size_t count = 3;
  int *values = (int *)std::malloc(count * sizeof(int));
  if (!values) std::abort();
  values[0] = 3; values[1] = 1; values[2] = 2;
  std::qsort(values, count, sizeof(int), compare);
  int valid = values[0] == 1 && values[2] == 3;
  std::free(values);
  std::exit(valid ? 0 : 1);
}
