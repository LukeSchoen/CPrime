// EXPECT_COMPILE_FAIL: 1
struct Owner { using PrivateToScope = int; };
PrivateToScope value;
int main() { return value; }
