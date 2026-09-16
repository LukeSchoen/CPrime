// EXPECT_COMPILE_FAIL: 1
template<class T> int target(T) { return T::missing; }
template<class T> int relay(T value) { return target(value); }
inline int unused() { return relay(1); }
int main() {}
