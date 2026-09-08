// EXPECT_COMPILE_FAIL: 1
struct Pointer { explicit operator int*() const; };
void test(Pointer p) { delete p; }
