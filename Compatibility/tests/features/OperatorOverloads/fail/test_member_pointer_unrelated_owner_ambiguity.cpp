// EXPECT_COMPILE_FAIL: 1
struct Base { int value; };
struct Left : Base {};
struct Right : Base {};
int select(int Left::*);
int select(int Right::*);
int main() { return select(&Base::value); }
