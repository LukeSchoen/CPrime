// EXPECT_EXIT: 0
template<class T> struct Value {
    template<class U> int read(U) { return 1; }
    template<class U> int read(U) const { return 2; }
};
template<> template<class U> int Value<char>::read(U);
template<> template<class U> int Value<char>::read(U) const;
int main() {
    Value<char> specialized;
    const Value<char> constant = {};
    Value<int> primary;
    return specialized.read(3) != 7 || specialized.read(2.5) != 7
        || primary.read(3) != 1 || constant.read(3) != 9;
}
template<> template<class Element>
int Value<char>::read(Element input) { return 7; }
template<> template<class Element>
int Value<char>::read(Element input) const { return 9; }
