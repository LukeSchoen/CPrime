// EXPECT_COMPILE_ARGS: -std=c++17
// CL gap probe: aggregate_base. A braced initializer for an aggregate with a
// public base initializes bases first, then members; no retained case exercised
// the base subobject in the list.

struct Base {
  int base;
};

struct Derived : Base {
  int member;
};

int main()
{
  Derived value{{1}, 2};
  return value.base == 1 && value.member == 2 ? 0 : 1;
}
