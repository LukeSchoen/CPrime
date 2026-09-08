// EXPECT_EXIT: 0
template<class T> struct Shape { enum { value = 1 }; };
template<class T, int N> struct Shape<T[N]> { enum { value = N + 10 }; };
template<class T, int N> struct Shape<const T[N]> { enum { value = N + 20 }; };
template<class T> struct Outer { enum { value = Shape<T>::value }; };
int main() {
    return Outer<int>::value != 1 || Outer<int[3]>::value != 13
        || Outer<const int[4]>::value != 24 || Outer<int[3]>::value != 13;
}
