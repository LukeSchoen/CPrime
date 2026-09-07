// EXPECT_COMPILE_FAIL: 1
template<class T> struct Forward;
struct Complete {};
Forward<Complete> object;
