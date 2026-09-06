// EXPECT_COMPILE_FAIL: 1
struct Record { template<class T> int value(T *x) const { return 1; } };
template<> int Record::value(int x) const { return 2; }
int main() { return 0; }
