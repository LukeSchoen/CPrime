// EXPECT_EXIT: 0
// Explicit template arguments select only members with the matching kind.
// The inherited type overload remains visible while the derived value overload
// replaces its base counterpart.
struct base {
  template <class T> int select() { return 1; }
  template <int N> int select() { return 2 + N; }
};

struct derived : base {
  using base::select;
  template <int N> int select() { return 4 + N; }
};

int main()
{
  base b;
  derived d;
  if (b.select<int>() != 1) return 1;
  if (b.select<1>() != 3) return 2;
  if (d.select<int>() != 1) return 3;
  if (d.select<1>() != 5) return 4;
  return 0;
}
