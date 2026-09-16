// EXPECT_COMPILE_FAIL: 1
template<class T, class U> constexpr int selected = 0;
template<class T, class U> constexpr int selected<T *, U> = 1;
template<class T, class U> constexpr int selected<T, U *> = 2;
int main() { return selected<int *, int *>; }
