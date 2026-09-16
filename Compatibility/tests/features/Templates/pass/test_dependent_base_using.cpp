// EXPECT_EXIT: 0
template<class T> struct Base {
    T value;
    T f(T x) { return value + x; }
};
template<class T> struct Derived : Base<T> {
    using Base<T>::value;
    using Base<T>::f;
    int run() { value = 35; return f(7); }
};
int main() { Derived<int> d; return d.run() != 42; }
