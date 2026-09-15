// EXPECT_COMPILE_FAIL: 1
template<class T> int target(T) { return T::missing; }
inline decltype(auto) unused() { return (target<int>); }
int main() {}
