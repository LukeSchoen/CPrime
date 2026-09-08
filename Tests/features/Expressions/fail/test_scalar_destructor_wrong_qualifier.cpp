// EXPECT_COMPILE_FAIL: 1
typedef double Wrong;
typedef int Integer;
void f(int value) { value.Wrong::~Integer(); }
