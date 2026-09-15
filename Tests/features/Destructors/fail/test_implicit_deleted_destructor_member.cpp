// EXPECT_COMPILE_FAIL: 1
struct Member { ~Member() = delete; };
struct Value { Member member; };
int main() { Value value; }
