// EXPECT_COMPILE_FAIL: 1
struct Object { int get(); int get(int); };
void (Object::*invalid)() = &Object::get;
