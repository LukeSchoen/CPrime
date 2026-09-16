// EXPECT_EXIT: 0
template<class First, class Second> struct Pair;
template<class A, class B = A> struct Pair;
template<class X = int, class Y> struct Pair { X first; Y second; };
template<class One, class Two> struct Pair;
template<int Count> struct Buffer;
template<int N = 3> struct Buffer { int values[N]; };
int main() {
    Pair<> value;
    value.first = 3;
    value.second = 7;
    return sizeof(value.first) != sizeof(int) || value.first + value.second != 10
        || sizeof(Buffer<>) != 3 * sizeof(int);
}
