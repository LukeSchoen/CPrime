// EXPECT_EXIT: 0
template<class T> int forward(T);
struct Owner {
    template<class U> friend int forward(U value) { return value + 3; }
    template<class U> friend int exposed(U);
};
template<class T> int exposed(T value) { return value + 7; }
template<class X, class Y> X first(X value, Y) { return value; }
struct Repeated { template<class X, class Y> friend X first(X, Y); };
template int first(int, double);
int main() { return forward(2) != 5 || exposed(4) != 11 || first(9, 2.0) != 9; }
