// EXPECT_COMPILE_FAIL: 1
struct Value {
    int read(int) = delete;
    int read(double) { return 0; }
};
int main() { Value value; return value.read(1); }
