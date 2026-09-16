// EXPECT_EXIT: 0
template<class T> struct Select {
    int value() { return 3; }
    template<class U> int member(U) { return 5; }
};
template<> int Select<int>::value() { return 7; }
template<> template<> int Select<int>::member<char>(char) { return 11; }
int main() {
    Select<int> a;
    Select<double> b;
    return a.value() != 7 || b.value() != 3 || a.member('a') != 11
           || a.member(1) != 5 || b.member('a') != 5;
}
