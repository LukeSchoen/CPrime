// EXPECT_COMPILE_ARGS: -std=c++17
// EXPECT_COMPILE_FAIL: 1
class Record { int value = 7; };
int main() { Record record; auto [value] = record; }
