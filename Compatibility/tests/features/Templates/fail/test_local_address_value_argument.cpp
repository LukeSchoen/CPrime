// EXPECT_COMPILE_FAIL: 1
template<class T, T P> int read() { return *P; }
int main() { int local = 5; return read<decltype(&local), &local>(); }
