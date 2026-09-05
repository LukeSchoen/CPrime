// EXPECT_EXIT: 0
struct Reader {
  template<int Rows, int Columns>
  int matrix(const int (&grid)[Rows][Columns]) {
    return Rows * 100 + Columns * 10 + grid[Rows - 1][Columns - 1];
  }
  template<int N> int later(int bias, const int (&values)[N]) {
    return bias + N + values[N - 1];
  }
  template<int N> int paired(const int (&a)[N], const int (&b)[N]) {
    return N + a[N - 1] + b[N - 1];
  }
  template<class T, int N> int typed(const T (&values)[N]) {
    return sizeof(T) * 100 + N * 10 + values[N - 1];
  }
};

struct Fallback {
  int paired(const int *, const int *) { return 20; }
  template<int N> int paired(const int (&a)[N], const int (&b)[N]) {
    return N;
  }
};

int main() {
  Reader reader;
  int grid[2][3] = {{1, 2, 3}, {4, 5, 6}};
  int a[2] = {2, 4}, other[3] = {1, 2, 3};
  short b[3] = {3, 5, 7};
  if (reader.matrix(grid) != 236) return 1;
  if (reader.later(1, a) != 7) return 2;
  if (reader.paired(a, a) != 10) return 3;
  if (reader.typed(a) != 424) return 4;
  if (reader.typed(b) != 237) return 5;
  if (reader.matrix(grid) != 236) return 6;
  Fallback fallback;
  return fallback.paired(a, other) != 20;
}
