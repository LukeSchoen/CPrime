// EXPECT_COMPILE_FAIL: 1
template<class T> void record(T);
struct Observer { friend void record<int>(char); };
int main() { return 0; }
