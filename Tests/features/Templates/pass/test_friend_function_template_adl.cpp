// EXPECT_EXIT: 0
namespace Example {
template<int N> struct Value {
    template<class T> friend int combine(Value, T value) { return N + value; }
};
template<class T> struct Reader {
    template<class U> friend int read(Reader, U value);
};
template<class T> struct Definition {
    template<class U> friend int read(Reader<T>, U value) { return value + 7; }
};
template struct Reader<int>;
template struct Definition<int>;
}
int main() {
    return combine(Example::Value<3>(), 5) != 8
        || combine(Example::Value<7>(), 2) != 9
        || read(Example::Reader<int>(), 4) != 11;
}
