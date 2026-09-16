// EXPECT_COMPILE_FAIL: 1
template<class T> struct Box { int get(T x) { return x.no_such_member(); } };
template struct Box<int>;
