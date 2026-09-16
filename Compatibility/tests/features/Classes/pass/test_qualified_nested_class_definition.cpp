// EXPECT_EXIT: 0
namespace Space { struct Outer; }
struct Space::Outer { struct Inner; };
struct Space::Outer::Inner { int value; };
int main() { Space::Outer::Inner x; x.value = 42; return x.value != 42; }
