// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -Werror
template<int Rows, int Columns>
int matrix(const int (&grid)[Rows][Columns])
{ return Rows * 100 + Columns * 10 + grid[Rows - 1][Columns - 1]; }

template<class T, int N>
int typed(const T (&values)[N])
{ return sizeof(T) * 100 + N * 10 + values[N - 1]; }

template<int N>
int paired(const int (&a)[N], const int (&b)[N])
{ return N + a[N - 1] + b[N - 1]; }
int paired(const int *, const int *) { return 20; }

template<int N>
int later(int bias, const int (&values)[N])
{ return bias + N + values[N - 1]; }

int main()
{
  int grid[2][3] = {{1, 2, 3}, {4, 5, 6}};
  int a[2] = {2, 4}, other[3] = {1, 2, 3};
  short b[3] = {3, 5, 7};
  if (matrix(grid) != 236 || matrix<2, 3>(grid) != 236) return 1;
  if (typed(a) != 424 || typed(b) != 237) return 2;
  if (later(1, a) != 7) return 3;
  if (paired(a, other) != 20) return 4;
  return 0;
}
