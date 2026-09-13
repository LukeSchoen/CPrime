// Must reject a genuine tag redefinition; no EXPECT_COMPILE_FAIL disguise.
struct S { int x; };
const int n = sizeof(struct S { int y; });
