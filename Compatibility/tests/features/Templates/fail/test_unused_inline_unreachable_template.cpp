// EXPECT_COMPILE_FAIL: 1
template<class T> int target(T) { return T::missing; }
inline int unused() {
    if (false) return target(1);
    return 0;
}
int main() {}
