// EXPECT_COMPILE_FAIL: 1
struct Point { Point(double, double) {} };
int select(Point, double=0.1) { return 0; }
int select(int) { return 1; }
int main() { Point a(1,2); return select({a,a}); }
