// EXPECT_EXIT: 0
template<class T> struct Value {
    template<class U> int read(U) { return 1; }
    int read() { return 3; }
};
template<> template<class U>
int Value<int>::read(U) { return 7; }
int main() {
    Value<int> specialized;
    Value<char> primary;
    return specialized.read(3) != 7 || specialized.read(2.5) != 7
        || primary.read(3) != 1 || specialized.read() != 3;
}
