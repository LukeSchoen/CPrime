// EXPECT_EXIT: 0
// A class-template argument that names an earlier parameter of the same
// parameter list cannot be deduced from the call argument, and the argument
// parser cannot see a parameter the replay has not declared yet.  Deduction
// binds the remaining parameters, then the substituted signature instantiates
// the id with that parameter in scope.
template <int> struct Extent { };

template <class T>
int measure(T a, Extent<sizeof(a)>) { return (int)sizeof(a); }

int main() {
  Extent<sizeof(int)> extent;
  Extent<sizeof(long long)> wide;
  if (measure(3, extent) != (int)sizeof(int)) return 1;
  return measure(3LL, wide) != (int)sizeof(long long);
}
