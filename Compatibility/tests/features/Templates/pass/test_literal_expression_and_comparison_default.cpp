// EXPECT_EXIT: 0
template<int N> struct Value { static const int value = N; };
template<int N, bool Small = N < 3> struct Limit { static const bool small = Small; };
template<int N> struct Derived : Value<10 / N> {};
int main() {
    return Value<10 / 2>::value != 5 || Value<1 + 2 * 3>::value != 7
        || Value<true && false>::value != 0 || !Limit<2>::small
        || Limit<4>::small || Derived<2>::value != 5;
}
