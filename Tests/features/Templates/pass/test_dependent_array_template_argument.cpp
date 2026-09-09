// EXPECT_EXIT: 0
template<int N, class T> struct Base { static int size() { return sizeof(T); } };
template<int N> struct Unused : Base<N, char[N]> {};
template<int N, int M> struct Value : Base<N, char[M + 1]> {};
template<class T, int N> struct Typed : Base<N, T[N]> {};
template<int N, int M> struct Matrix : Base<N, char[N][M]> {};
int main() { return Value<1, 2>::size() != 3 || Value<1, 4>::size() != 5
    || Typed<short, 3>::size() != 3 * sizeof(short) || Matrix<2, 3>::size() != 6; }
