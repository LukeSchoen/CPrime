// EXPECT_COMPILE_FAIL: 1
struct Point { Point(double, double) {} };
int main() { Point a(1,2); Point invalid{a,a}; }
