// EXPECT_EXIT: 0
template<int N> struct Sequence {
    int value;
    Sequence() : value(N) {}
    Sequence(const Sequence<N - 1> &previous) : value(previous.value + 1) {}
};
int main() {
    Sequence<4> a;
    Sequence<5> b(a);
    return b.value != 5;
}
