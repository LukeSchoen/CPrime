template<class T> struct ArraySummary {
    int count;
    T last;
    template<int N> ArraySummary(const T (&values)[N]);
};
template<class T> template<int N>
ArraySummary<T>::ArraySummary(const T (&values)[N])
    : count(N), last(values[N - 1]) {}

int select(int (&)[2]) { return 1; }
int select(const int (&)[2]) { return 2; }
int matrix(const int (&values)[2][3]) { return values[1][2]; }

int main() {
    int pair[2] = {4, 7};
    const int triple[3] = {3, 5, 9};
    const int fixed[2] = {1, 2};
    int grid[2][3] = {{1, 2, 3}, {4, 5, 6}};
    ArraySummary<int> first(pair), second(triple), third(pair);
    if (first.count != 2 || first.last != 7) return 1;
    if (second.count != 3 || second.last != 9) return 2;
    if (third.count != 2 || third.last != 7) return 3;
    if (select(pair) != 1 || select(fixed) != 2) return 4;
    return matrix(grid) != 6;
}
