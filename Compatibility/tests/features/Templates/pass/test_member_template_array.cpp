// Consolidated from similar standalone regressions; each cpc_case_N preserves one.

namespace cpc_case_0
{
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

int run() {
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
}

namespace cpc_case_1
{
template<class T> struct ArraySummary {
    int count;
    T last;
    template<int N> ArraySummary(const T (&values)[N]);
};
template<class T> template<int N>
ArraySummary<T>::ArraySummary(const T (&values)[N])
    : count(N), last(values[N - 1]) {}

int select(int (&)[2]) { return 1; }
int select(const int (&)[2]) { return 2; }
int matrix(const int (&values)[2][3]) { return values[1][2]; }

int run() {
    int pair[2] = {4, 7};
    const int triple[3] = {3, 5, 9};
    const int fixed[2] = {1, 2};
    int grid[2][3] = {{1, 2, 3}, {4, 5, 6}};
    ArraySummary<int> first(pair), second(triple), third(pair);
    if (first.count != 2 || first.last != 7) return 1;
    if (second.count != 3 || second.last != 9) return 2;
    if (third.count != 2 || third.last != 7) return 3;
    if (select(pair) != 1 || select(fixed) != 2) return 4;
    return matrix(grid) != 6;
}
}

namespace cpc_case_2
{
template<class T> struct Owner {
    template<class U> static char (&data(U*))[3] {
        static char buffer[3];
        return buffer;
    }
};
int run() {
    int value = 0;
    char (&buffer)[3] = Owner<int>::data(&value);
    buffer[2] = 17;
    return sizeof(Owner<int>::data(&value)) != 3
        || Owner<int>::data(&value)[2] != 17
        || &Owner<int>::data(&value) != &buffer;
}
}

int main()
{
  if (int code = cpc_case_0::run()) { return code; }
  if (int code = cpc_case_1::run()) { return code; }
  if (int code = cpc_case_2::run()) { return code; }
  return 0;
}
