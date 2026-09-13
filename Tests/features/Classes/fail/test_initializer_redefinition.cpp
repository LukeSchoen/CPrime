// EXPECT_COMPILE_FAIL: 1
/* A rejected constant-initializer probe must remain a hard diagnostic when
   the initializer is replayed dynamically. */
struct S { int x; };
const int n = sizeof(struct S { int y; });
