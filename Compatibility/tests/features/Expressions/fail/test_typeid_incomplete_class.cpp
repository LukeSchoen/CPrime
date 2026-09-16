// EXPECT_COMPILE_FAIL: 1
#include <typeinfo>
struct Incomplete;
int main() { typeid(Incomplete); }
