// EXPECT_COMPILE_FAIL: 1
struct Pointer {
    operator int*() const;
    operator char*() const;
};
void test(Pointer p) { delete p; }
