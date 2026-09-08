// EXPECT_EXIT: 0
template<class T, int N> int last(T (&values)[N]) { return values[N - 1] + N; }
template<class T, int N> int relay(T (&values)[N]) { return last(values); }
int main() {
    int small[2] = {4, 8}; const int large[5] = {1, 2, 3, 4, 9};
    return relay(small) != 10 || relay(large) != 14 || relay(small) != 10;
}
