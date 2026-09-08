struct Value { int n; };
int operator,(Value value, int n) { return value.n + n; }
struct Member {
    int operator,(int n) { return n + 20; }
};
template<int N> struct Vector { int n; };
template<int N> int operator,(Vector<N> value, int n) { return value.n + n + N; }
template<int N> Vector<N + 1> grow(Vector<N> value) {
    Vector<N + 1> result = {value.n + 1};
    return result;
}
int main() {
    Value value = {7};
    Member member;
    Vector<4> vector = {3};
    if ((value, 5) != 12) return 1;
    if ((member, 2) != 22) return 2;
    if ((vector, 6) != 13) return 3;
    Vector<5> larger = grow(vector);
    if (larger.n != 4) return 4;
    int x = 0;
    (0, x) = 9;
    return x != 9;
}

