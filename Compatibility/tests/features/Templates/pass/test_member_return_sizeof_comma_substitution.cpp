template<bool, class T> struct Enable { typedef T type; };
template<class T> struct Enable<false, T> {};
template<class T, class A, class B> struct Detect {
    typedef char Yes;
    struct No { char bytes[2]; };
    template<class U, class X, class Y>
    static typename Enable<(sizeof(U(X(), Y()), 1) > 0), Yes>::type test(int);
    template<class, class, class> static No test(...);
    enum { value = sizeof(test<T, A, B>(0)) == 1 };
};
struct Pair { Pair(int, int); };
int main() { return Detect<int, int, int>::value || !Detect<Pair, int, int>::value; }
