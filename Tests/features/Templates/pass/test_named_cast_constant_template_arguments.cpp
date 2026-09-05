// EXPECT_EXIT: 0
enum Kind { First, Second, Third };
template<Kind K, int Count> struct Choice { static const int result = K * 10 + Count; };
template<int N> int dependent_choice() { return Choice<static_cast<Kind>(N), (N + 1)>::result; }
template<int N> int negative_choice() { return Choice<static_cast<Kind>(N), -N>::result; }
int main() {
    if (Choice<static_cast<Kind>(1), 2>::result != 12) return 1;
    if (Choice<static_cast<Kind>(static_cast<int>(Second) + 1), (8 >> 1)>::result != 24) return 2;
    if (Choice<static_cast<Kind>((2 > 1) ? 0 : 2), (3 < 4 ? 7 : 8)>::result != 7) return 3;
    if (dependent_choice<2>() != 23) return 4;
    if (negative_choice<2>() != 18) return 5;
    return 0;
}
