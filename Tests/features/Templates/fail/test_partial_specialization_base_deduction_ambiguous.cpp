// EXPECT_COMPILE_FAIL: 1
struct Tag {};
template<class T, class U> struct Value {};
template<class T> struct Value<T, Tag> {};
struct Mixed : Value<int, Tag>, Value<double, Tag> {};
template<class T> void read(const Value<T, Tag>&);
int main() { Mixed value; read(value); }
