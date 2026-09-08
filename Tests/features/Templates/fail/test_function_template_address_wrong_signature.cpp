// EXPECT_COMPILE_FAIL: 1
void function(double);
template<void (*F)(int)> struct Dispatch {};
Dispatch<function> invalid;
