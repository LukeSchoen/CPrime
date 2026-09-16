// EXPECT_COMPILE_FAIL: 1
struct First { First(int) {} };
struct Second { Second(First) {} };
void consume(Second) {}
int main() { consume(1); }
