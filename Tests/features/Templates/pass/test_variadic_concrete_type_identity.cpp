// EXPECT_EXIT: 0
template<class... T> struct Tuple {};
int identify(Tuple<int, double>&) { return 1; }
int identify(Tuple<int, char>&) { return 2; }
int identify(Tuple<int>&) { return 3; }
int identify(Tuple<>&) { return 4; }
template<class... T> using Alias = Tuple<T...>;
int main() {
    Alias<int, double> first;
    Alias<int, char> second;
    Alias<int> single;
    Alias<> empty;
    if (identify(first) != 1) return 1;
    if (identify(second) != 2) return 2;
    if (identify(single) != 3) return 3;
    if (identify(empty) != 4) return 4;
    return 0;
}
