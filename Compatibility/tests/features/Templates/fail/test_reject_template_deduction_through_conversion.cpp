// EXPECT_COMPILE_FAIL: 1
template<class T> struct Box { Box(T); };
template<class T> int read(const Box<T> &) { return 1; }
int main() { return read(17); }
