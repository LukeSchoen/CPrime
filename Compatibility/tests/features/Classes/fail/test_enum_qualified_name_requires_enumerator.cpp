// EXPECT_COMPILE_FAIL: 1
enum Direction { Left, Right };
int Outside = 5;
int main() { return Direction::Outside; }
