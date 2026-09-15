// EXPECT_COMPILE_FAIL: 1
template<class T> int target(T) { return T::missing; }
inline int unused() { return target(1); }
int main() {}
