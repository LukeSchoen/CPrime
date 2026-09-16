// EXPECT_COMPILE_FAIL: 1
template<class T> struct Wrap { typedef T Type; };
template<class T> struct Value { typedef Wrap<T> Alias; };
int main() { return sizeof(Value<int>::Alias::Missing); }
