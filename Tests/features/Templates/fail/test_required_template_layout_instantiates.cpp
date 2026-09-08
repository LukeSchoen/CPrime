// EXPECT_COMPILE_FAIL: 1
template<class T> struct Invalid { typename T::missing field; };
Invalid<int> *pointer;
int main() { return sizeof(*pointer); }
