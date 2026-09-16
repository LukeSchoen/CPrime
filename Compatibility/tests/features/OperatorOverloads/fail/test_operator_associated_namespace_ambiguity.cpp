// EXPECT_COMPILE_FAIL: 1
namespace first { enum class A { value }; }
namespace second { enum class B { value }; }
namespace first { int operator+(A, second::B) { return 1; } }
namespace second { int operator+(first::A, B) { return 2; } }
int main() { return first::A::value + second::B::value; }
