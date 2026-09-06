// EXPECT_COMPILE_FAIL: 1
struct Size { int value; };
struct Window { Window(Size) {} };
int main() { if (false) { Window window("invalid"); } }
