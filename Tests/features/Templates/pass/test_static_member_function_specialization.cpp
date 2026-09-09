// EXPECT_EXIT: 0
template<class T> struct Value { static int read(); static int read(int); };
template<> int Value<int>::read() { return 7; }
template<> int Value<char>::read() { return 3; }
template<> int Value<int>::read(int n) { return n + 2; }
int main() { return Value<int>::read() != 7 || Value<char>::read() != 3
    || Value<int>::read(5) != 7; }
