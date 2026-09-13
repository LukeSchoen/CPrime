/* A GNU array designator in a saved template body is an initializer clause,
   not a lambda introducer.  The body rewrite marks every `[` that follows
   `{`, `,`, `(` or `=` as a lambda introducer, so replaying `{ [e] = 0 }`
   analyzed a capture named `e` and reported `lambda capture 'e' must name an
   automatic variable` (g++.dg/ext/desig11.C). */

enum { e = 2 };

template<int Index> int designated_values()
{
  const int x[] = { [e] = 7, [Index] = 9 };
  if (x[e] != 7)
    return 1;
  if (x[Index] != 9)
    return 2;
  if (x[0] != 0)
    return 3;
  return 0;
}

template<int Index> struct Table
{
  int value() const
  {
    const int y[3] = { [Index] = 11 };
    return y[Index];
  }

  int chained() const
  {
    const int grid[3][2] = { [e][1] = 5, [Index][0] = 6 };
    return grid[e][1] * 10 + grid[Index][0];
  }
};

template<int Index> int capture_still_works()
{
  int captured = Index + 1;
  auto lambda = [captured]() { return captured; };
  return lambda();
}

int main()
{
  if (designated_values<1>() != 0)
    return 1;
  if (designated_values<3>() != 0)
    return 2;

  Table<1> table;
  if (table.value() != 11)
    return 3;
  if (table.chained() != 56)
    return 4;

  Table<0> zero;
  if (zero.value() != 11)
    return 5;

  if (capture_still_works<1>() != 2)
    return 6;
  return 0;
}
