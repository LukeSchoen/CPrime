// EXPECT_COMPILE_FAIL: 1
inline void unused() {
    struct Local { int call() { return nonexistent_value; } };
    Local value;
    (void)value.call();
}
int main() {}
