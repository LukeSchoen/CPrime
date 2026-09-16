// EXPECT_COMPILE_FAIL: 1
template<int Number> struct Value {};
template<int Number> void previous(Value<Number - 1>);
int main() { previous<5>(Value<3>()); }
