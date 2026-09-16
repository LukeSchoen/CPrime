// EXPECT_COMPILE_ARGS: -std=c++17
// Coverage: GCC gcc-14.2.0 g++.dg/cpp1z/fold2.C.
// Unlike the upstream compile-only matrix, these operands avoid division by
// zero and every instantiated arithmetic/comparison result is checked.
#define CHECK(name, op, a, b, c, l, r) \
  template<class... T> int name##_l(T... x) { return (... op x); } \
  template<class... T> int name##_r(T... x) { return (x op ...); } \
  int name() { return name##_l(a,b,c) != l || name##_r(a,b,c) != r; }
CHECK(add, +, 8, 4, 2, 14, 14)
CHECK(sub, -, 8, 4, 2, 2, 6)
CHECK(mul, *, 8, 4, 2, 64, 64)
CHECK(divide, /, 16, 4, 2, 2, 8)
CHECK(mod, %, 17, 5, 3, 2, 1)
CHECK(band, &, 7, 3, 1, 1, 1)
CHECK(bor, |, 4, 2, 1, 7, 7)
CHECK(bxor, ^, 7, 3, 1, 5, 5)
CHECK(shl, <<, 1, 2, 1, 8, 16)
CHECK(shr, >>, 32, 2, 1, 4, 16)
CHECK(eq, ==, 2, 2, 1, 1, 0)
CHECK(ne, !=, 2, 2, 1, 1, 1)
CHECK(lt, <, 1, 2, 3, 1, 0)
CHECK(gt, >, 3, 2, 1, 0, 1)
CHECK(le, <=, 1, 2, 3, 1, 1)
CHECK(ge, >=, 3, 2, 1, 1, 1)
#define COMMA ,
CHECK(comma, COMMA, 1, 2, 3, 3, 3)
#define ASSIGN(name, op, initial, expected) \
  template<class... T> int name(T... x) { int v = initial; return (v op ... op x); } \
  int name##_check() { return name(4, 2) != expected; }
ASSIGN(assign, =, 32, 2)
ASSIGN(add_assign, +=, 32, 38)
ASSIGN(sub_assign, -=, 32, 26)
ASSIGN(mul_assign, *=, 32, 256)
ASSIGN(div_assign, /=, 32, 4)
ASSIGN(mod_assign, %=, 35, 1)
ASSIGN(and_assign, &=, 7, 0)
ASSIGN(or_assign, |=, 1, 7)
ASSIGN(xor_assign, ^=, 7, 1)
ASSIGN(shl_assign, <<=, 1, 64)
ASSIGN(shr_assign, >>=, 128, 2)
int main() {
  if (add() || sub() || mul() || divide() || mod()) return 1;
  if (band() || bor() || bxor() || shl() || shr()) return 2;
  if (eq() || ne() || lt() || gt() || le() || ge() || comma()) return 3;
  if (assign_check() || add_assign_check() || sub_assign_check()) return 4;
  if (mul_assign_check() || div_assign_check() || mod_assign_check()) return 5;
  if (and_assign_check() || or_assign_check() || xor_assign_check()) return 6;
  return shl_assign_check() || shr_assign_check();
}
