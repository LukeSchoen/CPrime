// EXPECT_COMPILE_FAIL: 1
#include <typeinfo>
int main() { std::type_info copy = typeid(int); }
