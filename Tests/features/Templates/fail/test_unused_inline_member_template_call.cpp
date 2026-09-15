// EXPECT_COMPILE_FAIL: 1
struct Owner {
    template<class T> static int target(T) { return T::missing; }
};
inline int unused() { return Owner::target(1); }
int main() {}
