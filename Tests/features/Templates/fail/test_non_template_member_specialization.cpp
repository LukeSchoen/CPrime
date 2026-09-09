// EXPECT_COMPILE_FAIL: 1
struct Value { struct Inner { int operator()(); }; };
template<> int Value::Inner::operator()() { return 3; }
