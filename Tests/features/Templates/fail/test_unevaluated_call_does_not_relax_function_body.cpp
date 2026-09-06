// EXPECT_COMPILE_FAIL: 1
struct Record { int value; };
template<class T> auto bad(T) { return T::value; }
typedef decltype(bad(Record())) Result;
int main() { return 0; }
