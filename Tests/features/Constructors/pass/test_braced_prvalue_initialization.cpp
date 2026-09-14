// EXPECT_COMPILE_ONLY: 1
// Declaration-only factories exercise initialization without requiring linkage.
struct S { S() = default; S(const S &) = delete; int x; };
S make();
S s{make()};

struct ret_type {
  ret_type() {}
  ret_type(const ret_type &) {}
  ~ret_type() {}
};
ret_type make_record() { return ret_type(); }
void initialize_record() { ret_type r3{make_record()}; }
