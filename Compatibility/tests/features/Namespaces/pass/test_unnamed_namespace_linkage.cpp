// EXPECT_COMPILE_ARGS: Compatibility/tests/features/Namespaces/pass/unnamed_namespace_other.cpp
namespace {
  int value = 7;
  struct Local { int n; };
  int read() { return value; }
}
template<class T> int local_value(T v) { return v.n; }
int other();
int main() { Local v = { 9 }; return read() != 7 || other() != 50 || local_value(v) != 9; }
